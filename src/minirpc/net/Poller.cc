#include "minirpc/net/Poller.h"
#include "minirpc/net/Channel.h"

namespace minirpc
{


Poller::Poller(EventLoop* loop) : ownerLoop_(loop) {}

bool Poller::hasChannel(Channel* ch) const {
    return channelMap_.find(ch->fd()) != channelMap_.end();
}


} // namespace minirpc
