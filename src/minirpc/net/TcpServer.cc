#include "minirpc/net/TcpServer.h"
#include "minirpc/common/logger.h"
#include "minirpc/core/RpcServer.h"
#include "minirpc/core/IConnection.h"
#include "minirpc/net/utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <signal.h>

#include "Nacos.h"
#include "minirpc/common/nacos_config.h"

namespace minirpc
{
    using namespace nacos;

    void register_services(const std::vector<std::string>& services, int server_port, const std::string& host) {
        Properties configProps;
        configProps[PropertyKeyConst::SERVER_ADDR] = host;
        INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
        ResourceGuard<INacosServiceFactory> _guardFactory(factory);

        auto g_namingSvc = factory->CreateNamingService();
        ResourceGuard<NamingService> _serviceGuard(g_namingSvc);

        LOG_INFO("register server instance %s:%d", host.c_str(), server_port);

        Instance instance;
        instance.clusterName = "DefaultCluster";
        instance.ip = host;
        instance.port = server_port;
        instance.ephemeral = true;

        try {
            for (const std::string& serviceName : services) {
                g_namingSvc->registerInstance(serviceName, instance);
            }
        } catch (NacosException &e) {
            throw std::runtime_error(std::string("Nacos registration failed: ") + e.what());
        }

        while (TcpServer::is_running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }


    std::atomic_bool TcpServer::is_running_ = true;

    TcpServer::TcpServer() = default;

    TcpServer::~TcpServer() {
        for (auto& [fd, pair] : connMap_) {
            delete pair.second; // Channel*
            // Conn is managed by shared_ptr
        }
    }

    void TcpServer::sigHandler(int sig) {
        is_running_.store(false);
    }

    int TcpServer::init(int port) {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            LOG_FATAL("create socket error");
        }

        int opt = 1;
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(sockfd_, reinterpret_cast<struct sockaddr*>(&address), sizeof address) < 0) {
            close(sockfd_);
            LOG_FATAL("bind error");
        }

        LOG_INFO("Listen port 0.0.0.0:%d", port);

        if (listen(sockfd_, 3) < 0) {
            close(sockfd_);
            LOG_FATAL("listen error");
        }

        set_nonblocking(sockfd_);
        return sockfd_;
    }

    void TcpServer::lunch_service_register(int port, const std::string& host) {
        std::thread worker(register_services, RpcServer::GetServices(), port, host);
        worker.detach();
    }

    void TcpServer::removeConn(int fd) {
        auto it = connMap_.find(fd);
        if (it == connMap_.end()) return;

        auto& [conn, ch] = it->second;
        // Disable events — no more callbacks will fire after this
        ch->disableAll();
        // Close socket so worker threads see the error
        ::close(fd);
        // Schedule Channel deletion on the EventLoop thread
        loop_->deferredDelete(ch);
        connMap_.erase(it);
    }

    void TcpServer::ClienHandler(TcpServer* server, std::shared_ptr<Conn> c) {
        int fd = c->fd();
        // ET mode: drain all available data
        while (true) {
            int len = c->readMsg();
            if (len < 0) {
                LOG_DEBUG("User %d disconnected.", fd);
                // Schedule removal on the EventLoop thread
                // (runInLoop would be ideal, but deferredDelete is safe from any thread)
                server->removeConn(fd);
                break;
            }
            if (len == 0) break;

            std::string body, srv_name;
            ProtocolHeader header;
            if (c->decode(body, srv_name, header)) {
                std::string res;
                if (RpcServer::Call(srv_name, body, res)) {
                    auto bytes = Encoder::Encode(header, res);
                    if (send(fd, bytes.data(), bytes.size(), 0) == -1) {
                        LOG_ERROR("send error");
                        break;
                    }
                }
                else {
                    header.code = FAILED;
                    auto bytes = Encoder::Encode(header, "");
                    send(fd, bytes.data(), bytes.size(), 0);
                }
            }
        }
    }


    void TcpServer::loop() {
        signal(SIGINT,  sigHandler);
        signal(SIGTERM, sigHandler);
        signal(SIGPIPE, SIG_IGN);

        loop_ = std::make_unique<EventLoop>();

        // Register listen socket with EventLoop
        auto listenConn = std::make_shared<Conn>(sockfd_);
        auto* listenCh = new Channel(loop_.get(), sockfd_);
        connMap_[sockfd_] = {listenConn, listenCh};

        listenCh->setReadEventCallback([this](const TimeStamp&) {
            sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);

            while (true) {
                int client_fd = accept(sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
                if (client_fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    LOG_ERROR("accept error");
                    break;
                }

                if (onConnectedCallBack_) {
                    onConnectedCallBack_(client_fd);
                }

                LOG_INFO("User %d Connected", client_fd);
                set_nonblocking(client_fd);

                auto conn = std::make_shared<Conn>(client_fd);
                auto* ch = new Channel(loop_.get(), client_fd);
                connMap_[client_fd] = {conn, ch};

                std::shared_ptr<Conn> connRef = conn;
                ch->setReadEventCallback([this, connRef](const TimeStamp&) {
                    threadPool_.enqueue(&TcpServer::ClienHandler, this, connRef);
                });
                ch->enableReading();
            }
        });
        listenCh->enableReading();

        LOG_INFO("EventLoop start, listening on fd=%d", sockfd_);

        loop_->loop();
    }

    void TcpServer::serve(int port) {
        init(port);

        std::string host = GetNacosServerHost();
        lunch_service_register(port, host);

        loop();
    }

} // namespace minirpc
