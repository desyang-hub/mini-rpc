#include "UserService.h"
#include "minirpc/common/Config.h"
#include "minirpc/common/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <getopt.h>
#include <iomanip>
#include <iostream>

using Clock = std::chrono::high_resolution_clock;

// Global statistics
static std::atomic<size_t> g_success{0};
static std::atomic<size_t> g_fail{0};
static std::atomic<int64_t> g_total_latency_us{0};

// Benchmark worker - each thread creates its own stub
void bench_worker(int requests)
{
    UserServiceProtobuf::UserServiceProtobuf_Stub stub;

    for (int i = 0; i < requests; ++i) {
        auto t0 = Clock::now();
        try {
            int sum = stub.add(100, 200);
            auto t1 = Clock::now();
            if (sum == 300) {
                g_success.fetch_add(1, std::memory_order_relaxed);
                g_total_latency_us.fetch_add(
                    std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count(),
                    std::memory_order_relaxed);
            } else {
                g_fail.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (...) {
            g_fail.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void print_usage(const char* prog)
{
    std::cout <<
        "Usage: " << prog << " [options]\n"
        "  -c <num>  concurrency (default: 100)\n"
        "  -n <num>  requests per thread (default: 100)\n"
        "  --help    show this help\n";
}

int main(int argc, char* argv[])
{
    auto cfg = minirpc::loadConfig();

    int concurrency = 100;
    int per_thread  = 100;

    int opt;
    while ((opt = getopt(argc, argv, "c:n:")) != -1) {
        switch (opt) {
            case 'c': concurrency = std::atoi(optarg); break;
            case 'n': per_thread  = std::atoi(optarg); break;
            default:  print_usage(argv[0]); return 1;
        }
    }

    int total = concurrency * per_thread;
    std::cout << "===== MiniRPC Benchmark =====\n"
              << "Concurrency:    " << concurrency << "\n"
              << "Per Thread:     " << per_thread << "\n"
              << "Total Requests: " << total << "\n"
              << "===============================\n";

    // Initialize RPC client
    minirpc::RpcClient::GetInstance().init(cfg.registry_address);

    // Allow Nacos subscription to sync
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Run benchmark
    auto t0 = Clock::now();
    {
        minirpc::ThreadPool pool(concurrency);
        for (int i = 0; i < concurrency; ++i) {
            pool.enqueue(bench_worker, per_thread);
        }
        // ThreadPool destructor waits for all tasks to complete
    }
    auto t1 = Clock::now();

    // Report results
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    size_t success = g_success.load();
    size_t fail    = g_fail.load();
    double qps     = (elapsed > 0) ? success / elapsed : 0;
    double avg_ms  = (success > 0) ? g_total_latency_us.load() / 1000.0 / success : 0;

    std::cout << "\n===== Results =====\n"
              << std::fixed << std::setprecision(2)
              << "Time:        " << elapsed << " s\n"
              << "Success:     " << success << "\n"
              << "Failed:      " << fail << "\n"
              << "QPS:         " << qps << "\n"
              << "Avg Latency: " << avg_ms << " ms\n"
              << "===================\n";

    return fail > 0 ? 1 : 0;
}
