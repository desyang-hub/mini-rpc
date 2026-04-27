#include <iostream>
#include <signal.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#include "Nacos.h"

using namespace nacos;

std::atomic<bool> g_running{true};
NamingService* g_namingSvc = nullptr;

// 信号处理函数
void signalHandler(int sig) {
    // std::cout << "\n🛑 收到信号 " << sig << "，准备退出..." << std::endl;
    g_running = false;
}

// 清理资源
void cleanup() {
    if (g_namingSvc) {
        try {
            // 可选：显式注销（虽然进程退出后 Nacos 也会自动剔除）
            for (int i = 0; i < 5; i++) {
                NacosString serviceName = "TestNamingService" + NacosStringOps::valueOf(i);
                g_namingSvc->deregisterInstance(serviceName, "127.0.0.1", 2000 + i);
                std::cout << "✅ 已注销 " << serviceName << std::endl;
            }
        } catch (...) {
            // 忽略注销异常
        }
    }
}

int tt() {
    // 注册信号处理器
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // kill 命令

    Properties configProps;
    configProps[PropertyKeyConst::SERVER_ADDR] = "127.0.0.1";
    INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
    ResourceGuard<INacosServiceFactory> _guardFactory(factory);
    
    g_namingSvc = factory->CreateNamingService();
    ResourceGuard<NamingService> _serviceGuard(g_namingSvc);

    Instance instance;
    instance.clusterName = "DefaultCluster";
    instance.ip = "127.0.0.1";
    instance.instanceId = "1";
    instance.ephemeral = true;

    // 注册服务
    try {
        for (int i = 0; i < 5; i++) {
            NacosString serviceName = "TestNamingService" + NacosStringOps::valueOf(i);
            instance.port = 2000 + i;
            g_namingSvc->registerInstance(serviceName, instance);
            std::cout << "✅ 已注册 " << serviceName << " on port " << (2000 + i) << std::endl;
        }
    } catch (NacosException &e) {
        std::cerr << "❌ 注册失败: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "🚀 服务已启动，等待信号退出 (Ctrl+C)..." << std::endl;

    // 主循环：等待退出信号
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 执行清理
    cleanup();
    std::cout << "👋 程序已退出。" << std::endl;
    return 0;
}

void register_services(const std::vector<std::string>& services, int server_port, const std::string& host="127.0.0.1") {
    Properties configProps;
    configProps[PropertyKeyConst::SERVER_ADDR] = host;
    INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
    ResourceGuard<INacosServiceFactory> _guardFactory(factory);
    
    g_namingSvc = factory->CreateNamingService();
    ResourceGuard<NamingService> _serviceGuard(g_namingSvc);

    // 注册服务
    try {
        for (const std::string& serviceName : services) {
            g_namingSvc->registerInstance(serviceName, host, server_port);
        }
    } catch (NacosException &e) {
        std::cerr << "❌ 注册失败: " << e.what() << std::endl;
        return;
    }

    std::cout << "🚀 服务已启动，等待信号退出 (Ctrl+C)..." << std::endl;

    // 主循环：等待退出信号
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


int main() {

    std::thread worker(tt);

    worker.detach();

    sleep(2);

    // worker.join();

    return 0;
}