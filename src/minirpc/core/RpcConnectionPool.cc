#include "RpcConnectionPool.h"
#include "minirpc/core/RpcClient.h"
#include "minirpc/core/utils.h"
#include "minirpc/net/utils.h"
#include "minirpc/common/logger.h"

#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdexcept>

namespace minirpc
{

IConnectionPool* IConnectionPool::GetConnectionPool(const std::string& server_name, const std::string& group_name) {
    return new RpcConnectionPool(server_name, group_name);
}

// 获取连接
IConnectionPtr RpcConnectionPool::connect() {
    // 优先检查本地直连地址（用于本地集成测试，跳过 Nacos）
    std::string addr = RpcClient::getLocalServiceAddress(server_name_);
    if (!addr.empty()) {
        int id = addr.find(':');
        if (id == std::string::npos) {
            throw std::runtime_error("Invalid address format, missing ':': " + addr);
        }
        std::string ip = addr.substr(0, id);
        std::string port = addr.substr(id + 1);
        return IConnection::GetConnection(ip, atoi(port.c_str()));
    }

    // 原有 Nacos 逻辑
    std::string ipAddr = getServiceAddress(server_name_);

    LOG_INFO("Service info: %s", ipAddr.c_str());

    int id = ipAddr.find(':');
    if (id == std::string::npos) {
        throw std::runtime_error("Invalid address format, missing ':': " + ipAddr);
    }
    std::string ip = ipAddr.substr(0, id);
    std::string port = ipAddr.substr(id + 1);

    // 发起连接
    return IConnection::GetConnection(ip, atoi(port.c_str()));
}


void RpcConnectionPool::addConnectionListener(IConnection* conn) {
    if (epollfd_ == -1) {
        // 1. 创建epoll
        epollfd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epollfd_ == -1) {
            LOG_FATAL("epoll create error");
        }

        // 2. 启动监听循环（joinable线程，由析构函数负责join）
        loop_thread_ = std::thread(&RpcConnectionPool::loop, this);
    }

    int sockfd = conn->fd();

    // 设置非阻塞
    set_nonblocking(sockfd);

    // 2. 注册fd监听到epoll
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // ET 触发，只要通知了就要一直读，直到空了(EAGAIN, EWOULDBLOCK)
    ev.data.ptr = conn;

    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        LOG_ERROR("add sockfd error, closing connection");
        conn->close();
        // 通知 caller 连接创建失败
    }
}


// 用于epoll管理连接
void RpcConnectionPool::loop() {
    std::vector<epoll_event> events(1024);

    while (pool_running_.load()) {
        int numEvents = epoll_wait(epollfd_, events.data(), events.size(), -1);

        int saveErrno = errno;

        if (numEvents == -1) {
            if (saveErrno == EINTR) continue;
            perror("epoll wait error");
            break;
        }

        for (int i = 0; i < numEvents; ++i) {
            IConnection* c = static_cast<IConnection*>(events[i].data.ptr);

            // 分发消息前校验连接健康，失效连接跳过避免空指针访问
            if (!c->isHealthy()) {
                LOG_DEBUG("Skipping event on unhealthy connection");
                continue;
            }

            if (message_handler_) {
                threadPool_.enqueue(message_handler_, c);
            }
        }

        if (numEvents == events.size()) {
            events.resize(2 * numEvents);
        }
    }

    if (epollfd_ != -1) {
        ::close(epollfd_);
        epollfd_ = -1;
    }
}

RpcConnectionPool::RpcConnectionPool(const std::string& server_name, const std::string& group_name) : server_name_(server_name), group_name_(group_name) {

}

RpcConnectionPool::~RpcConnectionPool() {
    pool_running_ = false;
    if (loop_thread_.joinable()) {
        loop_thread_.join();
    }
    // 清理池内剩余连接，避免 socket fd 泄漏
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connection_que_.empty()) {
        IConnectionPtr conn = std::move(connection_que_.front());
        connection_que_.pop();
        if (conn) {
            conn->close();
        }
    }
}




// 借连接：若池空且未达上限 → 新建；否则等待（初期可返回 nullptr）
IConnectionPtr RpcConnectionPool::getConnection() {
    IConnectionPtr conPtr{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (int i = 0; i < connection_que_.size(); ++i) {
            if (connection_que_.front()->isHealthy()) {
                conPtr = std::move(connection_que_.front());
                connection_que_.pop();
                break;
            }

            // 销毁不健康的连接
            connection_que_.pop();
        }
    }

    // 应该实现conn释放时，自动归还连接，获取失效就不用返回注册了
    if (conPtr == nullptr) {
        // 创建连接
        conPtr = connect();
        // 将创建的连接添加到消息监听
        addConnectionListener(conPtr.get());
        // 如果连接已失效（如 epoll 注册失败），直接丢弃
        if (!conPtr->isHealthy()) {
            LOG_ERROR("Connection failed after listener registration");
            return getConnection(); // 递归重试
        }
    }

    // 设置该连接依附的池，用于当连接销毁时，自动回到连接池中
    conPtr->set_master_pool(this);

    return conPtr;
}

// 归还连接：放回池中（不关闭），无效连接直接丢弃
void RpcConnectionPool::returnConnection(IConnection* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (conn && conn->isHealthy()) {
        connection_que_.emplace(conn);
    } else {
        // 无效连接直接关闭，不放回池中
        if (conn) {
            conn->close();
        }
    }
}

    
} // namespace minirpc
