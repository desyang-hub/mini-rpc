#include "minirpc/net/Poller.h"
#include "minirpc/net/EpollPoller.h"

#include <memory>

namespace minirpc
{


Poller* Poller::NewDefaultPoller(EventLoop* loop) {
    return new EpollPoller(loop);
}
    
} // namespace minirpc
