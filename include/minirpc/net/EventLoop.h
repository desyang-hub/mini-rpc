#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/net/Poller.h"

#include <memory>


namespace minirpc
{
    


class EventLoop : public nonecopyable
{
private:
    std::unique_ptr<Poller> poller_;
    ChannelList channels_;
public:
    EventLoop();
    ~EventLoop() = default;


public:
    void removeChannel(Channel* ch);
    void updateChannel(Channel* ch);

    void loop(); // 循环
};


} // namespace minirpc