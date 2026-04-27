#include "RpcConnectionPool.h"
#include "minirpc/core/utils.h"
#include "minirpc/net/utils.h"
#include "minirpc/common/logger.h"
#include "minirpc/core/RpcClient.h"

#include <sys/epoll.h>
#include <netinet/in.h>

namespace minirpc
{

IConnectionPool* IConnectionPool::GetConnectionPool(const std::string& server_name, const std::string& group_name) {
    return new RpcConnectionPool(server_name, group_name);
}

// 获取连接
IConnectionPtr RpcConnectionPool::connect() {
    // 目前只用了server_name
    std::string ipAddr = getServiceAddress(server_name_);

    int id = ipAddr.find(':');
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
        
        // 2. 暂时放在这里启动监听循环
        std::thread loop_woker(&RpcConnectionPool::loop, this);
        loop_woker.detach();
    }

    int sockfd = conn->fd();

    // 设置非阻塞
    set_nonblocking(sockfd);

    // 2. 注册fd监听到epoll
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // ET 触发，只要通知了就要一直读，直到空了(EAGAIN, EWOULDBLOCK)
    ev.data.ptr = conn;

    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        // perror("epoll add sockfd error");
        close(sockfd);
        // exit(-1);
        LOG_ERROR("add sockfd error");
    }
}


// 用于epoll管理连接
void RpcConnectionPool::loop() {
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
            IConnection* c = static_cast<IConnection*>(events[i].data.ptr);
            int fd = c->fd();


            // 这里是一个用户client_fd 事件
            // 最好使用线程池来进行调度，否则开销巨大
            // ClienHandler(this, c);
            // std::thread request_woker(&TcpServer::ClienHandler, this, c);
            // request_woker.detach();

            threadPool_.submit(std::bind(&RpcClient::messageHandler, &RpcClient::GetInstance(), c));
        }

        // 如果事件数量太多了，自动进行扩容
        if (numEvents == events.size()) {
            events.resize(2 * numEvents);
        }
    }
}

RpcConnectionPool::RpcConnectionPool(const std::string& server_name, const std::string& group_name) : server_name_(server_name), group_name_(group_name) {

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
    }

    // 设置该连接依附的池，用于当连接销毁时，自动回到连接池中
    conPtr->set_master_pool(this);

    return conPtr;
}

// 归还连接：放回池中（不关闭）
void RpcConnectionPool::returnConnection(IConnection* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_que_.emplace(conn);
}

    
} // namespace minirpc
