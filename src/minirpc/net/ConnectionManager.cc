/**
 * @FilePath     : /mini-rpc/src/minirpc/net/ConnectionManager.cc
 * @Description  : ConnectionManager implementation
 * @Author       : desyang
 * @Date         : 2026-06-10 11:46:52
**/
#include "minirpc/net/ConnectionManager.h"
#include "minirpc/net/TcpClient.h"
#include "minirpc/net/EndPoint.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/RpcException.h"
#include <memory>
#include <queue>

#include <iostream>

namespace minirpc
{

class ConnectionManager::Impl
{
private:
    std::unordered_map<EndPoint, std::queue<TcpClientPtr>> tcpClients_;
    mutable std::mutex mutex_;
    muduo::net::MessageCallback messageCallback_;
    std::atomic<size_t> cnt_{0};
    std::condition_variable condition_;

public:
    TcpClientPtr getConnection(const EndPoint& ep, ConnectionManager* connMgr)
    {
        // 如果连接本来就存在，那么就直接返回可用连接
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tcpClients_.find(ep);
            if (it != tcpClients_.end() && !it->second.empty())
            {
                auto t = std::move(it->second.front());
                it->second.pop();
                return t;
            }
        }

        // 如果连接不存在，那么创建连接
        auto newTcpClient = std::make_shared<TcpClient>(ep, connMgr);

        // 设置消息回调
        newTcpClient->setMessageCallback(messageCallback_);

        newTcpClient->Start();

        return newTcpClient;
    }

    TcpClientPtr getConnection(const std::vector<EndPoint> &eps, ConnectionManager* connMgr)
    {
        if (eps.empty()) {
            throw RpcException("Not Found Service Instance.");
        }
            
        auto checker = [this, &eps] {
            for (const auto &ep : eps) {
                auto it = tcpClients_.find(ep);
                if (it != tcpClients_.end() && !it->second.empty())
                {
                    return true;
                }
            }
            return false;
        };

        {
            std::unique_lock<std::mutex> lock(mutex_);
            bool flag = false;
            if (!condition_.wait_for(lock, std::chrono::seconds(1), 
                [this, &flag, &eps, &checker] {
                    flag = checker();
                    return cnt_.load() < 10 || flag;
                })) {
                throw RpcException("Timeout error");
            }
            
            if (flag) {
                for (const auto &ep : eps) {
                    auto it = tcpClients_.find(ep);
                    if (it != tcpClients_.end() && !it->second.empty())
                    {
                        auto conn = std::move(it->second.front()); 
                        it->second.pop();
                        return conn;
                    }
                }
            } else {
                cnt_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        

        // 如果连接本来就存在，那么就直接返回可用连接
        // {
        //     std::lock_guard<std::mutex> lock(mutex_);

        //     // 只要有一个存在就直接返回
        //     for (const auto &ep : eps)
        //     {
        //         auto it = tcpClients_.find(ep);
        //         if (it != tcpClients_.end() && !it->second.empty())
        //         {
        //             auto conn = std::move(it->second.front()); 
        //             it->second.pop();
        //             return conn;
        //         }
        //     }
        // }

        // cnt_.fetch_add(1, std::memory_order_relaxed);

        // 如果连接不存在，那么创建连接
        auto newTcpClient = std::make_shared<TcpClient>(eps[0], connMgr);

        if (!messageCallback_)
        {
            throw RpcException("message Callback is nullptr");
        }

        // 设置消息回调
        newTcpClient->setMessageCallback(messageCallback_);
        newTcpClient->Start();

        return newTcpClient;
    }

    void setMessageCallback(muduo::net::MessageCallback cb)
    {
        if (!cb)
        {
            LOG_ERROR("setMessageCallback called with empty callback!");
            return;
        }
        messageCallback_ = std::move(cb);
    }

    void recovery(TcpClientPtr ptr) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tcpClients_[ptr->endPoint()].push(ptr);
        }
        condition_.notify_one();
    }
};

// ====== ConnectionManager implementation ======

ConnectionManager::ConnectionManager()
    : impl_(std::make_unique<Impl>())
{
}

ConnectionManager::~ConnectionManager() = default;

TcpClientPtr ConnectionManager::getConnection(const EndPoint &ep)
{
    return impl_->getConnection(ep, this);
}

TcpClientPtr ConnectionManager::getConnection(const std::vector<EndPoint> &eps)
{
    return impl_->getConnection(eps, this);
}

void ConnectionManager::setMessageCallback(muduo::net::MessageCallback cb)
{
    impl_->setMessageCallback(std::move(cb));
}


void ConnectionManager::recovery(TcpClientPtr ptr) {
    impl_->recovery(ptr);
}

} // namespace minirpc
