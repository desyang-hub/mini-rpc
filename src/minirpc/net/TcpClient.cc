/**
 * @FilePath     : /mini-rpc/src/minirpc/net/TcpClient.cc
 * @Description  : TcpClient implementation
 * @Author       : desyang
 * @Date         : 2026-06-10 11:46:52
 **/
#include "minirpc/net/TcpClient.h"
#include "minirpc/net/EndPoint.h"
#include "minirpc/common/logger.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/net/ConnectionManager.h"

#include <iostream>
#include <unistd.h>

#include <future>

namespace minirpc
{
// ====== TcpClient implementation ======

TcpClient::TcpClient(const EndPoint &ep, ConnectionManager* connMgr)
    : loop_(), is_connected_(false), ep_(ep), connMgr_(connMgr)
{
    muduo::net::EventLoop *loop = loop_.startLoop();

    client_ = std::make_shared<muduo::net::TcpClient>
    (loop, muduo::net::InetAddress(ep.host.c_str(), ep.port), "MyClient");

    client_->setConnectionCallback(
        [this](const muduo::net::TcpConnectionPtr &conn) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (conn->connected()) {
                    conns_ = conn;
                    is_connected_ = true;
                } else {
                    conns_.reset();
                    is_connected_ = false;
                }
            }
            condition_.notify_all();
        });
}

TcpClient::~TcpClient()
{
    muduo::net::EventLoop* loop = nullptr;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (client_) {
            loop = client_->getLoop();
        }
        // ⚠️ 关键：先释放我们对 TcpConnection 的引用
        // muduo TcpClient::~TcpClient() 中会检查 connection_.unique()：
        //   - unique() == true  → 调用 forceClose()（强制断开）
        //   - unique() == false → 不调用 forceClose()（认为还有其他人引用）
        // 如果我们的 conns_ 还活着，unique() == false，muduo 不会强制关闭，
        // 导致 TcpConnection 以 kConnected 状态被销毁 → 触发 assert(state_ == kDisconnected)
        conns_.reset();
    }

    if (client_) {
        // muduo TcpClient 设计上允许在主线程调用 disconnect()，
        // 但实际清理（removeConnection, connectDestroyed, channel_->remove）
        // 都是通过 runInLoop/queueInLoop 异步投递到 IO 线程执行的。
        //
        // muduo TcpClient::~TcpClient() 流程：
        //   1. 复制 connection_ shared_ptr（增加引用计数）
        //   2. runInLoop(setCloseCallback) — 异步
        //   3. 如果 unique() → forceClose() → queueInLoop(forceCloseInLoop) — 异步
        //   4. ~TcpClient 返回，muduo 端的 connection_ 引用释放
        //
        // 策略：先 reset client_（触发 muduo ~TcpClient 投递清理任务），
        // 然后通过 CountDownLatch 在 IO 线程中等待，确保所有 pending functors
        // （setCloseCallback, forceCloseInLoop, handleClose, connectDestroyed,
        //  channel_->remove）都已执行完毕后，再退出 EventLoop。

        client_.reset();  // 触发 muduo TcpClient 析构，投递所有清理任务

        if (loop && !loop->isInLoopThread()) {
            muduo::CountDownLatch latch(1);
            loop->runInLoop([&latch]() {
                // 屏障：执行到这里时，所有之前通过 runInLoop/queueInLoop 投递的
                // 任务都已在当前 poll 循环的 doPendingFunctors() 中执行完毕。
                latch.countDown();
            });
            latch.wait();
        }

        // 退出 IO 线程 — 此时 TcpConnection 已 kDisconnected，Channel 已 remove
        if (loop) {
            loop->quit();
        }
    }
    // loop_ 析构时会 join IO 线程（loop_->quit() 已在上一步调用）
}

void TcpClient::Start()
{
    client_->connect();
}

void TcpClient::setMessageCallback(muduo::net::MessageCallback cb)
{
    if (!cb)
    {
        LOG_ERROR("getConnection: messageCallback_ is empty! RpcClient may be destroyed.");
    }
    client_->setMessageCallback(std::move(cb));
}

void TcpClient::sendRequest(const std::string &req)
{
    return sendRequest(req.c_str(), req.size());
}

void TcpClient::sendRequest(const void *data, size_t len)
{
    muduo::net::TcpConnectionPtr conn;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_.wait_for(lock, std::chrono::seconds(1),
                [this] { return conns_ && conns_->connected(); })) {
            throw RpcException("Request Timeout: connection not ready.");
        }
        conn = conns_;
    }

    auto data_copy = std::make_shared<std::string>((const char*)data, len);
    conn->getLoop()->runInLoop([conn, data_copy]() {
        if (conn->connected()) {
            conn->send(*data_copy);
        }
    });
}


void TcpClient::recovery() {
    if (connMgr_ != nullptr) {
        connMgr_->recovery(shared_from_this());
    }
}

} // namespace minirpc
