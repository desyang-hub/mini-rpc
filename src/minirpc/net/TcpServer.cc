#include "minirpc/net/TcpServer.h"
#include "minirpc/common/logger.h"
#include "minirpc/core/RpcServer.h"
#include "minirpc/net/utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <signal.h>

#include "Nacos.h"

namespace minirpc
{
    using namespace nacos;

    void register_services(const std::vector<std::string>& services, int server_port, const std::string& host) {
        // 注册信号处理器
        // signal(SIGINT, signalHandler);   // Ctrl+C
        // signal(SIGTERM, signalHandler);  // kill 命令
    
        Properties configProps;
        configProps[PropertyKeyConst::SERVER_ADDR] = host;
        INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
        ResourceGuard<INacosServiceFactory> _guardFactory(factory);
        
        auto g_namingSvc = factory->CreateNamingService();
        ResourceGuard<NamingService> _serviceGuard(g_namingSvc);
    
        Instance instance;
        instance.clusterName = "DefaultCluster";
        instance.ip = host;
        instance.port = server_port;
        instance.instanceId = "1";
        instance.ephemeral = true;
    
        // 注册服务
        try {
            for (const std::string& serviceName : services) {
                // std::cout << "name: " << serviceName << std::endl;
                g_namingSvc->registerInstance(serviceName, instance);
            }
        } catch (NacosException &e) {
            // std::cerr << "❌ 注册失败: " << e.what() << std::endl;
            exit(-1);
        }
    
        // std::cout << "🚀 服务已启动，等待信号退出 (Ctrl+C)..." << std::endl;
    
        // 主循环：等待退出信号
        while (TcpServer::is_running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }


    std::atomic_bool TcpServer::is_running_ = true;

    TcpServer::TcpServer() {

    }

    TcpServer::~TcpServer() {

    }

    void TcpServer::sigHandler(int sig) {
        is_running_.store(false);
    }

// 初始化, 创建sockfd
int TcpServer::init(int port) {
    // 1. 创建socket
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        LOG_FATAL("create socket error");
        // perror("create socket error");
        // exit(-1);
    }

    // 2. 设置地址复用
    int opt = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    // 3. bind
    struct sockaddr_in  address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有请求
    address.sin_port = htons(port);

    if (bind(sockfd_, (struct sockaddr*)&address, sizeof address) < 0) {
        // perror("bind error");
        close(sockfd_);
        LOG_FATAL("create socket error");
        // exit(-1);
    }

    LOG_INFO("Listen port 0.0.0.0:%d", port);
    // std::cout << "Listen port 0.0.0.0:" << port << std::endl;

    // listen
    if (listen(sockfd_, 3) < 0) { // 第二个参数表示的是如果等待的数量超过3，直接拒绝
        // perror("listen error");
        close(sockfd_);
        // exit(-1);
        LOG_FATAL("listen error");
    }

    return sockfd_;
}


/**
 * @brief 启用服务注册子线程
 */
void TcpServer::lunch_service_register(int port, const std::string& host) {

    auto& services = RpcServer::GetServices();
    // LOG_INFO("name: %s:", services[0].c_str());
    // std::cout << services.size() << std::endl;
    
    std::thread worker(register_services, RpcServer::GetServices(), port, host);
    worker.detach();
}


// 移除该连接
void TcpServer::removeConn(Conn* c) {
    auto it = connMap_.find(c->fd());

    // not exists
    if (it == connMap_.end()) {
        close(c->fd());
        return;
    }

    if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, c->fd(), nullptr) == -1) {
        LOG_ERROR("epoll ctl error");
    }
    close(c->fd());
    connMap_.erase(it);
    delete c;
}

void TcpServer::ClienHandler(TcpServer* server, Conn* c) {
    // 这里做的就是io处理逻辑，默认使用ET触发模式，需要一次处理完数据

    int cli_fd = c->fd();

    // ET, 必须要一次处理完，直到缓存空了
    // 接收用户消息，并写回
    while (true) {
        int len = c->readMsg();
        
        // 出错了，或断开了
        if (len < 0) {
            LOG_DEBUG("User %d disconnected.", c->fd());
            server->removeConn(c);
            break;
        }
        else if (len == 0) {
            LOG_DEBUG("continue recv message.");
            break;
        }
        else {
            // TODO: 这里理论上应该放回调，当读入了消息就该回调接口

            // 解码读取消息，拷贝消息
            std::string body;
            std::string srv_name;

            ProtocolHeader header;
            // 是否成功解码
            bool is_success = c->decode(body, srv_name, header);
            if (is_success) {
                std::string res;
                is_success = RpcServer::Call(srv_name, body, res);

                if (!is_success) {
                    header.code = FAILED;
                    res = "";
                }

                auto bytes = Encoder::Encode(header, res);

                // 将消息写回, 通过包发送
                if (send(c->fd(), bytes.data(), bytes.size(), 0) == -1) {
                    LOG_ERROR("send error");
                    break;
                }
            }
        }
    }
}


void TcpServer::loop() {
    signal(SIGINT,  &TcpServer::sigHandler);   // Ctrl+C
    signal(SIGTERM, &TcpServer::sigHandler);  // kill 命令

    // 1. 创建epoll
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd_ < 0) {
        // perror("epoll create error");
        close(sockfd_);
        LOG_FATAL("epoll create error");
    }

    // 2. 注册fd监听到epoll
    epoll_event ev;
    connMap_[sockfd_] = new Conn(sockfd_);
    ev.events = EPOLLIN | EPOLLET; // ET 触发，只要通知了就要一直读，直到空了(EAGAIN, EWOULDBLOCK)
    ev.data.ptr = connMap_[sockfd_];
    
    LOG_INFO("Epoll ET Mode epfd=%d", epollfd_);
    // std::cout << "Epoll ET Mode epfd=" << epollfd << std::endl;

    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd_, &ev) < 0) {
        // perror("epoll add sockfd error");
        close(sockfd_);
        close(epollfd_);
        // exit(-1);
        LOG_FATAL("epoll add sockfd error");
    }

    // 3. 进行事件监听
    // epoll_event events[1024];
    std::vector<epoll_event> events(1024);
    

    while (is_running_.load()) {
        // 阻塞等待，返回触发的事件个数
        int numEvents = epoll_wait(epollfd_, events.data(), events.size(), -1); // -1 表示无限等待

        int saveErrno = errno;

        if (numEvents == -1) {
            if (saveErrno == EINTR) continue; // 被信号中断
            perror("epoll wait error");
            break;
        }

        // 处理被激活的事件
        for (int i = 0; i < numEvents; ++i) {
            Conn* c = static_cast<Conn*>(events[i].data.ptr);
            int fd = c->fd();

            if (fd == sockfd_) { // 这个是sockfd的用户请求事件
                sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                int client_fd = accept(fd, (sockaddr*)&client_addr, &len);

                if (client_fd < 0) {
                    // accept失败
                    continue;
                }

                if (onConnectedCallBack_) {
                    onConnectedCallBack_(client_fd);
                }
                
                LOG_INFO("User %d Connected", client_fd);
                connMap_[client_fd] = new Conn(client_fd);

                set_nonblocking(client_fd); // 设置非阻塞

                // 将事件注册到epoll
                epoll_event ev;
                ev.events = EPOLLIN | EPOLLET;
                // ev.data.fd = client_fd;
                ev.data.ptr = connMap_[client_fd];

                if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                    close(client_fd);
                    continue;
                }
            }
            else {
                // 这里是一个用户client_fd 事件

                // 最好使用线程池来进行调度，否则开销巨大
                // ClienHandler(this, c);
                // std::thread request_woker(&TcpServer::ClienHandler, this, c);
                // request_woker.detach();
                
                threadPool_.enqueue(&TcpServer::ClienHandler, this, c);
                // threadPool_.submit(std::bind(&TcpServer::ClienHandler, this, c));
            }
        }

        // 如果事件数量太多了，自动进行扩容
        if (numEvents == events.size()) {
            events.resize(2 * numEvents);
        }
    }

}

// 服务方法
void TcpServer::serve(int port) {
    // 创建socket
    int sockfd = init(port);

    std::string host = "127.0.0.1";

    lunch_service_register(port, host);

    // 创建epoll
    loop();
}




    
} // namespace minirpc
