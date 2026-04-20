#include "minirpc/net/EventLoop.h"
#include "minirpc/net/Channel.h"

#include <algorithm>

namespace minirpc
{

    EventLoop::EventLoop() {

    }

// 移除Channel
void EventLoop::removeChannel(Channel* ch) {
    poller_->removeChannel(ch);

    auto it = std::find(channels_.begin(), channels_.end(), ch);

    if (it != channels_.end()) {
        channels_.erase(it);
        delete ch; // 将资源释放
    }
}

// 更新Channel
void EventLoop::updateChannel(Channel* ch) {
    poller_->updateChannel(ch);
}

    
} // namespace minirpc
