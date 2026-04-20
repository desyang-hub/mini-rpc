#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/timeStamp.h"

#include <vector>
#include <unordered_map>

namespace minirpc
{

// 前置声明，暂时不需要
class Channel;
class EventLoop;

using ChannelList = std::vector<Channel*>;

class Poller : public nonecopyable
{
public:
    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual TimeStamp poll(int timeoutMs, ChannelList* activateChannel) = 0;
    virtual void updateChannel(Channel* ch) = 0;
    virtual void removeChannel(Channel* ch) = 0;

    bool hasChannel(Channel*) const;

    // 可以通过该接口获取Poller的具体实现
    static Poller* NewDefaultPoller(EventLoop* loop);

protected:
    // key socket fd, value Channel*
    using ChannelMap = std::unordered_map<int, Channel*>;

    EventLoop* ownerLoop_;
    ChannelMap channelMap_;
};
    
} // namespace minirpc
