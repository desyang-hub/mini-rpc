#include "minirpc/core/RpcClient.h"

#include "minirpc/core/IConnection.h"
#include "minirpc/core/IConnectionPoolFactory.h"

#include <thread>
#include <sys/eventfd.h>


namespace minirpc
{
    
RpcClient::RpcClient() 
    : sockfd_(-1), 
    request_id_(0), 
    wakeup_fd_(-1),
    connnection_pool_factory_(IConnectionPoolFactory::CreateConnectionPoolFactory()) {
    wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd_ == -1) {
        LOG_FATAL("eventfd create error");
    }
    // loop_woker_ = std::thread(&RpcClient::ReadLoop, this);
}

RpcClient::~RpcClient() {
    is_running_.store(false);
    // 唤醒 epoll_wait
    if (wakeup_fd_ != -1) {
        uint64_t val = 1;
        write(wakeup_fd_, &val, sizeof(val));
    }
    // 等待工作线程退出
    if (loop_woker_.joinable()) {
        loop_woker_.join();
    }
    // 关闭资源
    close(wakeup_fd_);
    close(sockfd_);
    close(epollfd_);
}

RpcClient& RpcClient::GetInstance() {
    static RpcClient instance;
    return instance;
}

void RpcClient::messageHandler(IConnection* c) {
    // 这里做的就是io处理逻辑，默认使用ET触发模式，需要一次处理完数据
    int cli_fd = c->fd();

    // ET, 必须要一次处理完，直到缓存空了
    // 接收用户消息，并写回
    while (true) {
        int len = c->readMsg();
        
        // 出错了，或断开了
        if (len < 0) {
            LOG_DEBUG("User %d disconnected.", c->fd());
            c->close();
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

            ProtocolHeader header;

            bool is_success = c->decode(header, body);
            if (is_success) {
                auto it = promiseMap_.find(header.request_id);
                assert(it != promiseMap_.end() && "Assert error: promise not found");
                it->second.set_value({header.code, body});                    
            }
        }
    }
}
    
} // namespace minirpc
