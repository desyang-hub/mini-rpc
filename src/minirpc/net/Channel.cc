#include "minirpc/net/Channel.h"
#include "minirpc/common/logger.h"
#include "minirpc/net/EventLoop.h"

#include <sys/epoll.h>

namespace minirpc
{

// static class member variable initial
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;


Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {

}

Channel::~Channel() {

}

/**
 * 改变事件后，需要在poller更改fd的事件epoll_ctl
 */
void Channel::update() {
    // 通过channel所属的EventLoop的调用Poller的相应方法，注册fd事件
    loop_->updateChannel(this);
    // loop_->updateChannel(this);
}

void Channel::remove() {
    // 在channel所属的EventLoop中删除channels中的this channel
    loop_->removeChannel(this);
}


void Channel::handleEventWithGuard(const TimeStamp& timeStamp) {
    // 根据接收到的事件执行相应的事件回调

    // 发生异常close
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeEventCallback_) {
            closeEventCallback_();
        }
    }

    // 发生了错误
    if (revents_ & EPOLLERR) {
        if (errorEventCallback_) {
            errorEventCallback_();
        }
    }

    // 读事件发生
    if (revents_ & kReadEvent) {
        if (readEventCallback_) {
            readEventCallback_(timeStamp);
        }
    }

    // 写事件发生
    if (revents_ & kWriteEvent) {
        if (writeEventCallback_) {
            writeEventCallback_();
        }
    }

}

// fd 得到poller通知后，进行事件处理
void Channel::handleEvent(const TimeStamp& timeStamp) {

    LOG_INFO("Channel::handleEvent revents: %d", revents_);

    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();

        // 确保相关的对象并没有释放
        if (guard) {
            handleEventWithGuard(timeStamp);
        }
    }
    else {
        // 无需绑定直接执行即可
        handleEventWithGuard(timeStamp);
    }
}


} // namespace minirpc