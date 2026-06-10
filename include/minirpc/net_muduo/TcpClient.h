
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

#include "minirpc/net_muduo/EndPoint.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/RpcException.h"

namespace minirpc
{

// 后续操作是通过配置文件来进行远程服务注册中心查询可用实例，并进一步获取实例地址，进行连接

class TcpClient {
private:
    muduo::net::EventLoopThread loopThread_;
    muduo::net::InetAddress serverAddr_;
    muduo::net::MessageCallback messageCallBack_;
    std::unique_ptr<muduo::net::TcpClient> clients_;
    muduo::net::TcpConnectionPtr conns_; // 与 clients_ 一一对应


    std::mutex mutex_;
    std::condition_variable condition_;
    bool is_connected;


public:
    explicit TcpClient(const EndPoint& ep) : loopThread_(), is_connected(false) {
        // ✅ EventLoopThread 内部会启动一个专属线程
        //    并在该线程中创建 EventLoop + 调用 loop()
        // muduo::net::EventLoopThread loopThread;
        
        // startLoop() 会阻塞直到子线程中的 EventLoop 创建完毕
        // 返回的指针指向子线程中的 Loop，但只用于传递，不在主线程操作
        muduo::net::EventLoop* loop = loopThread_.startLoop();
        
        // TcpClient 可以在主线程构造，但传入的是子线程的 Loop
        // connect() 内部会通过 runInLoop 将实际连接操作投递到子线程
        clients_ = std::make_unique<muduo::net::TcpClient>(loop, muduo::net::InetAddress(ep.host.c_str(), ep.port), "MyClient");

        clients_->setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn){
            std::lock_guard<std::mutex> lock(mutex_);
            if (conn->connected()) {
                conns_ = conn;       // ✅ 连接建立，保存
                is_connected = true;
            } else {
                conns_.reset();      // ✅ 连接断开，清除
                // 可选：触发重连逻辑、通知连接管理器该 Endpoint 不可用等
                is_connected = false;
            }
        });
    }

    void Start() {
        clients_->connect();
    }

    void setMessageCallback(muduo::net::MessageCallback cb) {
        if (!cb) {
            LOG_ERROR("getConnection: messageCallback_ is empty! RpcClient may be destroyed.");
        }
        clients_->setMessageCallback(std::move(cb));
    }

    void sendRequest(const std::string& req) {
        return sendRequest(req.c_str(), req.size());
    }

    void sendRequest(const void* data, size_t len) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_.wait_for(lock, std::chrono::seconds(3), [this]{
            return is_connected;
        })) {
            // 超时逻辑
            throw RpcException("Request Timeouts Error.");
        }

        conns_->getLoop()->runInLoop([this, req = std::string((const char*)data, len)]() {
            if (conns_->connected()) {
                conns_->send(req);
            }
        });
    }
};

using TcpClientPtr = std::shared_ptr<TcpClient>;

} // namespace minirpc