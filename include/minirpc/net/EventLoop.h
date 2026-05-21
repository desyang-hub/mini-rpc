#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/net/Poller.h"

#include <memory>
#include <atomic>
#include <mutex>
#include <vector>


namespace minirpc
{

class EventLoop : public nonecopyable
{
private:
    std::unique_ptr<Poller> poller_;
    ChannelList activeChannels_;
    std::atomic<bool> quit_{false};

    // Deferred channel deletions from worker threads
    std::mutex deferredMutex_;
    std::vector<Channel*> deferredDelete_;

public:
    EventLoop();
    ~EventLoop() = default;

    void loop();
    void quit();

    void removeChannel(Channel* ch);
    void updateChannel(Channel* ch);

    // Thread-safe: schedule channel for deletion after current dispatch
    void deferredDelete(Channel* ch);
};

} // namespace minirpc