#include <iostream>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <getopt.h> // POSIX getopt

#include "UserService.h"
#include "user.pb.h"
#include "minirpc/common/ThreadPool.h"
#include "minirpc/common/Config.h"

// 全局统计变量
std::atomic<size_t> g_success{0};
std::atomic<size_t> g_fail{0};
std::atomic<int64_t> g_total_latency_us{0};

// 压测工作线程
void bench_worker(size_t requests_per_thread) {
    // 每个线程独立创建 Stub，避免多线程竞争
    UserServiceProtobuf::UserServiceProtobuf_Stub stub; 

    for (size_t i = 0; i < requests_per_thread; ++i) {
        int a = 10, b = 5;
        auto start = std::chrono::high_resolution_clock::now();
        try {
            int sum = stub.add(a, b);
            auto end = std::chrono::high_resolution_clock::now();
            
            if (sum == a + b) {
                g_success.fetch_add(1, std::memory_order_relaxed);
                int64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                g_total_latency_us.fetch_add(latency, std::memory_order_relaxed);
            } else {
                g_fail.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (const std::exception& e) {
            g_fail.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

// 打印使用说明
void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -c <num>   并发数 (Concurrency)，默认: 100\n"
              << "  -n <num>   单线程请求数 (Per Thread Requests)，默认: 100\n"
              << "  -h <host>  RPC Server Host，默认: 127.0.0.1\n"
              << "  -p <port>  RPC Server Port，默认: 8083\n"
              << "  --help     显示此帮助信息\n";
}

int main(int argc, char* argv[]) {
    // 0. Load config
    auto cfg = minirpc::loadConfig();

    // 1. 默认参数
    int concurrency = 100;
    int total_requests = 10000;
    int per_thread_requests = 100;
    std::string host = "127.0.0.1";
    int port = cfg.port;

    // 2. 使用 getopt 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "c:n:h:p:")) != -1) {
        switch (opt) {
            case 'c': concurrency = std::atoi(optarg); break;
            case 'n': per_thread_requests = std::atoi(optarg); break;
            case 'h': host = optarg; break;
            case 'p': port = std::atoi(optarg); break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    total_requests = per_thread_requests * concurrency;

    // 3. 打印压测配置
    std::cout << "================ MiniRPC Benchmark ================" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "Concurrency: " << concurrency << ", Total Requests: " << total_requests << std::endl;
    std::cout << "===================================================" << std::endl;

    std::chrono::_V2::system_clock::time_point global_start;
    // 4. Init RpcClient with Nacos address from config
    minirpc::RpcClient::GetInstance().init(cfg.registry_address);

    // 5. 启动线程池并分发任务
    {
        minirpc::ThreadPool pool(concurrency);
        size_t base_reqs = total_requests / concurrency;
        size_t remainder = total_requests % concurrency;

        global_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < concurrency; ++i) {
            // size_t reqs = base_reqs + (i < static_cast<int>(remainder) ? 1 : 0);
            pool.enqueue(bench_worker, base_reqs);
        }
    }

    // 5. 等待所有任务完成（依赖 ThreadPool 析构时的 join）
    auto global_end = std::chrono::high_resolution_clock::now();
    double total_seconds = std::chrono::duration<double>(global_end - global_start).count();
    
    // 6. 计算并输出压测报告
    size_t total_success = g_success.load();
    size_t total_fail = g_fail.load();
    int64_t total_latency = g_total_latency_us.load();
    double avg_latency_ms = (total_success > 0) ? (total_latency / 1000.0 / total_success) : 0;
    double qps = (total_seconds > 0) ? (total_success / total_seconds) : 0;

    // 3. 打印压测配置
    std::cout << "================ MiniRPC Benchmark ================" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "Concurrency: " << concurrency << ", Total Requests: " << total_requests << std::endl;
    std::cout << "===================================================" << std::endl;
    std::cout << "\n================ Benchmark Results ================" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Time:      " << total_seconds << " s" << std::endl;
    std::cout << "Success:         " << total_success << std::endl;
    std::cout << "Failed:          " << total_fail << std::endl;
    std::cout << "QPS:             " << qps << std::endl;
    std::cout << "Avg Latency:     " << avg_latency_ms << " ms" << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}