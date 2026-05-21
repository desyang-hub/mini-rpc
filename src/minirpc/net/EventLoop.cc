#include "minirpc/net/EventLoop.h"
#include "minirpc/net/Channel.h"
#include "minirpc/net/Poller.h"
#include "minirpc/common/logger.h"

#include <algorithm>

namespace minirpc
{

EventLoop::EventLoop()
    : poller_(Poller::NewDefaultPoller(this)) {
}

void EventLoop::loop() {
    while (!quit_.load()) {
        activeChannels_.clear();
        poller_->poll(-1, &activeChannels_);

        for (Channel* ch : activeChannels_) {
            ch->handleEvent(TimeStamp::Now());
        }

        // Process deferred channel deletions (from worker threads)
        std::vector<Channel*> toDelete;
        {
            std::lock_guard<std::mutex> lock(deferredMutex_);
            toDelete.swap(deferredDelete_);
        }
        for (Channel* ch : toDelete) {
            delete ch;
        }
    }
}

void EventLoop::quit() {
    quit_.store(true);
}

void EventLoop::deferredDelete(Channel* ch) {
    std::lock_guard<std::mutex> lock(deferredMutex_);
    deferredDelete_.push_back(ch);
}

void EventLoop::removeChannel(Channel* ch) {
    poller_->removeChannel(ch);
}

void EventLoop::updateChannel(Channel* ch) {
    poller_->updateChannel(ch);
}

} // namespace minirpc
