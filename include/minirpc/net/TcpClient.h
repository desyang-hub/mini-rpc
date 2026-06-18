/**
 * @FilePath     : /mini-rpc/include/minirpc/net/TcpClient.h
 * @Description  : TcpClient header
 * @Author       : desyang
 * @Date         : 2026-06-10 11:46:52
**/
#pragma once

#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/EventLoopThread.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>

#include "minirpc/net/EndPoint.h"
#include "minirpc/common/nonecopyable.h"

#include <iostream>

namespace minirpc
{

class ConnectionManager;

class TcpClient : public nonecopyable, public std::enable_shared_from_this<TcpClient>
{
public:
    TcpClient(const EndPoint &ep, ConnectionManager* connMgr = nullptr);
    ~TcpClient();

    void Start();

    void setMessageCallback(muduo::net::MessageCallback cb);

    void sendRequest(const std::string &req);

    void sendRequest(const void *data, size_t len);

    void recovery();

    EndPoint& endPoint() {
        return ep_;
    }

    const EndPoint& endPoint() const {
        return ep_;
    }

private:
    EndPoint ep_;
    ConnectionManager* connMgr_;
    muduo::net::MessageCallback messageCallBack_;
    std::shared_ptr<muduo::net::TcpClient> client_;
    muduo::net::EventLoopThread loop_;
    muduo::net::TcpConnectionPtr conns_; // 与 clients_ 一一对应

    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> is_connected_;
};

using TcpClientPtr = std::shared_ptr<TcpClient>;

} // namespace minirpc
