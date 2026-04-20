#pragma once

#include "minirpc/net/Poller.h"

#include <vector>
#include <sys/epoll.h>

/**
 * epoll_create
 * 
 * epoll_ctl add/del/mod
 * 
 * epoll_wait
 */
namespace minirpc
{

class EventLoop;

class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller();

    TimeStamp poll(int timeoutMs, ChannelList* activateChannel) override;
    void updateChannel(Channel* ch) override;
    void removeChannel(Channel* ch) override;

private:
    using EventList = std::vector<struct epoll_event>;

    static const int kInitEventListSize;

    void fillActivateChannels(int numEvents, ChannelList* channels) const;

    // 更新channel
    void update(int operation, Channel* chan);

private:
    int epollfd_;
    EventList events_;
};

} // namespace minirpc
