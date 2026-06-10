/**
 * @FilePath     : /mini-rpc/include/minirpc/net_muduo/ConnectionManager.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-10 11:46:52
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 15:38:58
**/
#pragma once

#include "minirpc/net_muduo/TcpClient.h"
#include "minirpc/net_muduo/EndPoint.h"
#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/logger.h"
#include <unordered_map>
#include <mutex>
#include <muduo/net/Callbacks.h>
#include <vector>

#include "minirpc/common/RpcException.h"

namespace minirpc 
{

// 连接管理器，用户可以通过List<EndPoint> 来获取
class ConnectionManager : public nonecopyable
{
private:
    std::unordered_map<EndPoint, TcpClientPtr> tcpClients_;
    mutable std::mutex mutex_;
    muduo::net::MessageCallback messageCallback_;

public:
    ConnectionManager() = default;
    ~ConnectionManager() = default;

    TcpClientPtr getConnection(const EndPoint& ep) {
        // 如果连接本来就存在，那么就直接返回可用连接
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tcpClients_.find(ep);
            if (it != tcpClients_.end()) {
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


    TcpClientPtr getConnection(const std::vector<EndPoint>& eps) {
        if (eps.empty()) throw RpcException("Not Found Service Instance.");

        // 如果连接本来就存在，那么就直接返回可用连接
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // 只要有一个存在就直接返回
            for (const auto& ep : eps) {
                auto it = tcpClients_.find(ep);
                if (it != tcpClients_.end()) {
                    return it->second;
                }
            }

            // 如果连接不存在，那么创建连接
            auto newTcpClient = std::make_shared<TcpClient>(eps[0]);
            tcpClients_[eps[0]] = newTcpClient;

            if (!messageCallback_) {
                throw RpcException("message Callback is nullptr");
            }

            // 设置消息回调
            newTcpClient->setMessageCallback(messageCallback_);
            newTcpClient->Start();

            return newTcpClient;
        }
        
    }

    void setMessageCallback(muduo::net::MessageCallback cb) {
        if (!cb) {
            LOG_ERROR("setMessageCallback called with empty callback!");
            return;
        }
        messageCallback_ = std::move(cb);
    }
};
    
} // namespace minirpc
