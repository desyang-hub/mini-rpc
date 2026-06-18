#include "minirpc/net/ConnectionManager.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/logger.h"

#include <queue>

namespace minirpc
{

class ConnectionManager::Impl
{
private:
    std::unordered_map<EndPoint, std::queue<TcpClientPtr>> pool_;
    mutable std::mutex mutex_;
    muduo::net::MessageCallback messageCallback_;
    std::atomic<size_t> cnt_{0};
    std::condition_variable condition_;

public:
    TcpClientPtr getConnection(const EndPoint& ep, ConnectionManager* mgr)
    {
        // Check pool first
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pool_.find(ep);
            if (it != pool_.end() && !it->second.empty()) {
                auto conn = std::move(it->second.front());
                it->second.pop();
                // Clean up empty queues
                if (it->second.empty()) pool_.erase(it);
                return conn;
            }
        }

        // Create new connection
        auto conn = std::make_shared<TcpClient>(ep, mgr);
        conn->setMessageCallback(messageCallback_);
        conn->Start();
        return conn;
    }

    TcpClientPtr getConnection(const std::vector<EndPoint>& eps, ConnectionManager* mgr)
    {
        if (eps.empty()) {
            throw RpcException("No service instances available");
        }

        // Wait for pooled connection or connection slot
        bool usePooled = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, std::chrono::seconds(1), [this, &eps, &usePooled] {
                for (const auto& ep : eps) {
                    auto it = pool_.find(ep);
                    if (it != pool_.end() && !it->second.empty()) {
                        usePooled = true;
                        return true;
                    }
                }
                return cnt_.load() < 10;
            });
        }

        // Return pooled connection if available
        if (usePooled) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& ep : eps) {
                auto it = pool_.find(ep);
                if (it != pool_.end() && !it->second.empty()) {
                    auto conn = std::move(it->second.front());
                    it->second.pop();
                    if (it->second.empty()) pool_.erase(it);
                    return conn;
                }
            }
        }

        // Round-robin to create new connection
        size_t idx = cnt_.fetch_add(1, std::memory_order_relaxed) % eps.size();
        auto conn = std::make_shared<TcpClient>(eps[idx], mgr);

        if (!messageCallback_) {
            throw RpcException("Message callback not set");
        }

        conn->setMessageCallback(messageCallback_);
        conn->Start();
        return conn;
    }

    void setMessageCallback(muduo::net::MessageCallback cb)
    {
        if (!cb) {
            LOG_ERROR("setMessageCallback called with empty callback");
            return;
        }
        messageCallback_ = std::move(cb);
    }

    void recovery(TcpClientPtr ptr)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_[ptr->endPoint()].push(std::move(ptr));
        }
        condition_.notify_one();
    }
};

// --- ConnectionManager facade ---

ConnectionManager::ConnectionManager()
    : impl_(std::make_unique<Impl>()) {}

ConnectionManager::~ConnectionManager() = default;

TcpClientPtr ConnectionManager::getConnection(const EndPoint& ep)
{
    return impl_->getConnection(ep, this);
}

TcpClientPtr ConnectionManager::getConnection(const std::vector<EndPoint>& eps)
{
    return impl_->getConnection(eps, this);
}

void ConnectionManager::setMessageCallback(muduo::net::MessageCallback cb)
{
    impl_->setMessageCallback(std::move(cb));
}

void ConnectionManager::recovery(TcpClientPtr ptr)
{
    impl_->recovery(std::move(ptr));
}

} // namespace minirpc
