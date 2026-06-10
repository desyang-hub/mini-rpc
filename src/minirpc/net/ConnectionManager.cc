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

namespace minirpc
{

class ConnectionManager::Impl
{
private:
    std::unordered_map<EndPoint, TcpClientPtr> tcpClients_;
    mutable std::mutex mutex_;
    muduo::net::MessageCallback messageCallback_;

public:
    TcpClientPtr getConnection(const EndPoint& ep)
    {
        // 如果连接本来就存在，那么就直接返回可用连接
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tcpClients_.find(ep);
            if (it != tcpClients_.end())
            {
                return it->second;
            }

            // 如果连接不存在，那么创建连接
            auto newTcpClient = std::make_shared<TcpClient>(ep);
            tcpClients_[ep] = newTcpClient;

            // 设置消息回调
            newTcpClient->setMessageCallback(messageCallback_);

            newTcpClient->Start();

            return newTcpClient;
        }
    }

    TcpClientPtr getConnection(const std::vector<EndPoint> &eps)
    {
        if (eps.empty())
            throw RpcException("Not Found Service Instance.");

        // 如果连接本来就存在，那么就直接返回可用连接
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // 只要有一个存在就直接返回
            for (const auto &ep : eps)
            {
                auto it = tcpClients_.find(ep);
                if (it != tcpClients_.end())
                {
                    return it->second;
                }
            }

            // 如果连接不存在，那么创建连接
            auto newTcpClient = std::make_shared<TcpClient>(eps[0]);
            tcpClients_[eps[0]] = newTcpClient;

            if (!messageCallback_)
            {
                throw RpcException("message Callback is nullptr");
            }

            // 设置消息回调
            newTcpClient->setMessageCallback(messageCallback_);
            newTcpClient->Start();

            return newTcpClient;
        }
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
};

// ====== ConnectionManager implementation ======

ConnectionManager::ConnectionManager()
    : impl_(std::make_unique<Impl>())
{
}

ConnectionManager::~ConnectionManager() = default;

TcpClientPtr ConnectionManager::getConnection(const EndPoint &ep)
{
    return impl_->getConnection(ep);
}

TcpClientPtr ConnectionManager::getConnection(const std::vector<EndPoint> &eps)
{
    return impl_->getConnection(eps);
}

void ConnectionManager::setMessageCallback(muduo::net::MessageCallback cb)
{
    impl_->setMessageCallback(std::move(cb));
}

} // namespace minirpc
