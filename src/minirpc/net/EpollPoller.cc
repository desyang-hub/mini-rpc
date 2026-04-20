#include "minirpc/net/EpollPoller.h"
#include "minirpc/net/Channel.h"
#include "minirpc/common/logger.h"

#include <errno.h>
#include <unistd.h>

namespace minirpc
{

namespace {
    // channel 未添加到poller中
    const int kNew = -1;
    // channel 已经添加到poller中
    const int kAdded = 1;
    // channel 从poller中删除
    const int kDeleted = 2;
}

const int EpollPoller::kInitEventListSize = 1024;

EpollPoller::EpollPoller(EventLoop* loop) : Poller(loop),
    events_(kInitEventListSize),
    epollfd_(epoll_create1(EPOLL_CLOEXEC)) 
{
    if (epollfd_ == -1) {
        LOG_FATAL("epoll create failed!");
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}



TimeStamp EpollPoller::poll(int timeoutMs, ChannelList* activateChannel) {
    LOG_INFO("func=EpollPoller::poll, activateChannel count=%lu", activateChannel->size());

    int numEvents = epoll_wait(epollfd_, events_.data(), events_.size(), timeoutMs);

    int curErrno = errno;
    TimeStamp now = TimeStamp::Now();

    if (numEvents == -1) {
        if (curErrno == EINTR) {
            LOG_ERROR("epoll_wait error");
        }
    }
    else if (numEvents == 0) {
        LOG_INFO("Nothing Event happing.");
    }
    else {
        LOG_INFO("numEvent=%u happing.", numEvents);
        fillActivateChannels(numEvents, activateChannel);

        // 进行扩容
        if (numEvents == events_.size()) {
            events_.resize(2 * numEvents);
        }
    }

    return now;
}

// 更新channel
void EpollPoller::updateChannel(Channel* ch) {
    const int index = ch->index();
    const int fd = ch->fd();

    LOG_INFO("fd=%d, events=%d, index=%d", fd, ch->events(), index);

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            channelMap_[fd] = ch;
        }

        ch->set_index(kAdded);
        update(EPOLL_CTL_ADD, ch);
    }
    else {
        if (ch->isNoneEvent()) {
            update(EPOLL_CTL_DEL, ch);
            ch->set_index(kDeleted);
        }
        else {
            update(EPOLL_CTL_MOD, ch);
        }
    }
}

// 移除channel
void EpollPoller::removeChannel(Channel* ch) {
    // 移除
    const int fd = ch->fd();
    channelMap_.erase(fd);

    // 更改状态
    const int index = ch->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, ch);
    }
    ch->set_index(kDeleted);
}


void EpollPoller::fillActivateChannels(int numEvents, ChannelList* channels) const {
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
        ch->setREvents(events_[i].events); // 设置真实发生的事件
        channels->push_back(ch); // EventLoop 拿到了Poller触发的Channel列表
    }
}

// 更新channel
void EpollPoller::update(int operation, Channel* chan) {

    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = chan->events();
    ev.data.ptr = chan;
    const int fd = chan->fd();

    if (epoll_ctl(epollfd_, operation, fd, &ev) < 0) {
        // 不影响运行
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("EPOLL_CTL_DEL error");
        }
        else {
            LOG_FATAL("EpollPoller::update() Fatal found.");
        }
    }
}
    
} // namespace minirpc
