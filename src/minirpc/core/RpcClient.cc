#include "minirpc/core/RpcClient.h"

#include <thread>
#include <sys/eventfd.h>


namespace minirpc
{
RpcClient::RpcClient() : sockfd_(Dial(8080)), request_id_(0), wakeup_fd_(-1) {
    wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd_ == -1) {
        LOG_FATAL("eventfd create error");
    }
    loop_woker_ = std::thread(&RpcClient::ReadLoop, this);
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
    
} // namespace minirpc
