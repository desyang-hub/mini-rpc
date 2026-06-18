#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

#include <muduo/net/Callbacks.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>

#include "minirpc/common/nonecopyable.h"
#include "minirpc/net/EndPoint.h"

namespace minirpc
{

class ConnectionManager;

class TcpClient : public nonecopyable, public std::enable_shared_from_this<TcpClient>
{
public:
    TcpClient(const EndPoint& ep, ConnectionManager* connMgr = nullptr);
    ~TcpClient();

    void Start();
    void setMessageCallback(muduo::net::MessageCallback cb);
    void sendRequest(const std::string& data);
    void sendRequest(const void* data, size_t len);
    void recovery();

    EndPoint& endPoint() { return ep_; }
    const EndPoint& endPoint() const { return ep_; }

private:
    EndPoint ep_;
    ConnectionManager* connMgr_ = nullptr;
    muduo::net::MessageCallback messageCallBack_;
    std::shared_ptr<muduo::net::TcpClient> client_;
    muduo::net::EventLoopThread loop_;
    muduo::net::TcpConnectionPtr conns_;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> is_connected_{false};
};

using TcpClientPtr = std::shared_ptr<TcpClient>;

} // namespace minirpc
