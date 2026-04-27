#include "minirpc/net/utils.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <signal.h>
#include <unistd.h>

#include "Nacos.h"

namespace minirpc
{

/**
 * 将文件描述符设置为非阻塞模式
 * @param fd 需要设置的文件描述符
 */
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get error");
        return;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}  

// 用于发起Tcp连接，返回fd
int Dial(int port, const std::string& host) {
    // 先建立连接，拿到fd
    // 1. 创建socket
    int clifd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 目标addr
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(clifd, (sockaddr*)&server_addr, sizeof server_addr) < 0) {
        perror("connect error");
        close(clifd);
        exit(-1);
    }

    return clifd;
}


using namespace nacos;

// 匿名命名空间
namespace {
    std::atomic<bool> g_running{true};
    NamingService* g_namingSvc = nullptr;

    void signalHandler(int sig) {
        g_running.store(false);
    }
}


/**
 * @brief 将服务注册到服务中心
 * 
 * @param services: 需要注册的服务
 * @param server_potr: 服务端口
 * @param host: 服务主机
 */
// void register_services(const std::vector<std::string>& services, int server_port, const std::string& host) {
//     // 注册信号处理器
//     signal(SIGINT, signalHandler);   // Ctrl+C
//     signal(SIGTERM, signalHandler);  // kill 命令

//     Properties configProps;
//     configProps[PropertyKeyConst::SERVER_ADDR] = host;
//     INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
//     ResourceGuard<INacosServiceFactory> _guardFactory(factory);
    
//     g_namingSvc = factory->CreateNamingService();
//     ResourceGuard<NamingService> _serviceGuard(g_namingSvc);

//     Instance instance;
//     instance.clusterName = "DefaultCluster";
//     instance.ip = host;
//     instance.instanceId = "1";
//     instance.ephemeral = true;

//     // 注册服务
//     try {
//         for (const std::string& serviceName : services) {
//             std::cout << "name: " << serviceName << std::endl;
//             g_namingSvc->registerInstance(serviceName, instance);
//         }
//     } catch (NacosException &e) {
//         std::cerr << "❌ 注册失败: " << e.what() << std::endl;
//         exit(-1);
//     }

//     // std::cout << "🚀 服务已启动，等待信号退出 (Ctrl+C)..." << std::endl;

//     // 主循环：等待退出信号
//     while (g_running.load()) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     }

//     // 程序退出
//     exit(-1);
// }
    
} // namespace minirpc
