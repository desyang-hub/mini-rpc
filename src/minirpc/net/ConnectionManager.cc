#include "minirpc/net/ConnectionManager.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/logger.h"

#include <queue>
#include <iostream>

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
    size_t activeCount_{0};

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

        bool usePooled = false;
        EndPoint selectedEp;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // ✅ 等待直到：有可用 pooled 连接 OR 有创建新连接的配额
            bool gotSlot = condition_.wait_for(lock, std::chrono::seconds(1), 
                [this, &eps, &usePooled, &selectedEp] {
                    // 优先检查连接池
                    for (const auto& ep : eps) {
                        auto it = pool_.find(ep);
                        if (it != pool_.end() && !it->second.empty()) {
                            usePooled = true;
                            selectedEp = ep;
                            return true;
                        }
                    }
                    // ✅ 关键：在锁内完成 "检查 + 预留" 的原子操作
                    if (activeCount_ < 10) {
                        ++activeCount_;  // 在锁内递增，杜绝竞态
                        return true;
                    }
                    return false;
                });
            
            // ✅ 超时且未获取到任何资源，拒绝而非放行
            if (!gotSlot) {
                throw RpcException("getConnection timed out: no pooled connection and max active connections reached");
            }
            
            // 如果命中连接池，在锁内取出
            if (usePooled) {
                auto it = pool_.find(selectedEp);
                auto conn = std::move(it->second.front());
                it->second.pop();
                if (it->second.empty()) pool_.erase(it);
                return conn;  // 注意：pooled 连接不消耗 activeCount_ 配额
            }
        }
        // 走到这里说明已在锁内预占了 activeCount_ 名额

        // Round-robin 选择端点（用单独的原子计数器，与限流解耦）
        size_t idx = cnt_.fetch_add(1, std::memory_order_relaxed) % eps.size();
        auto conn = std::make_shared<TcpClient>(eps[idx], mgr);

        if (!messageCallback_) {
            --activeCount_;  // ✅ 失败时归还配额
            condition_.notify_one();
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
