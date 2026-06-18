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
    muduo::net::TcpConnectionPtr conn;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (client_) {
            loop = client_->getLoop();
        }
        conn = conns_;
        conns_.reset();
    }

    if (client_) {
        if (loop && !loop->isInLoopThread()) {
            // muduo ~TcpClient() pendingFunctors_ 执行顺序分析：
            //
            // ~TcpClient() 投递：
            //   A: runInLoop(setCloseCallback)   — 设置 closeCallback = removeConnection
            //   B: queueInLoop(forceCloseInLoop)  — handleClose → closeCallback() →
            //                                            removeConnection() →
            //                                            queueInLoop(connectDestroyed)
            //
            //   connectDestroyed → channel_->remove() — 这才是设置 addedToLoop_ = false 的地方
            //
            // 所以 pendingFunctors_ 顺序：
            //   1. setCloseCallback
            //   2. forceCloseInLoop  → 内部 queueInLoop(connectDestroyed) → 追加到队尾
            //   3. latch1.countDown  (我们投递的屏障1)
            //   4. connectDestroyed  (由 forceCloseInLoop 内部追加)
            //
            // 我们的 latch1 在 connectDestroyed 之前执行！所以需要在 connectDestroyed
            // 之后再加一个屏障。
            //
            // 解决：在 pendingFunctors_ 中先投递我们的屏障 C（在 connectDestroyed 之前），
            // 但 connectDestroyed 会在 forceCloseInLoop 内部被 queueInLoop 追加。
            // 所以屏障 C 在位置 3，connectDestroyed 在位置 4。
            //
            // 正确做法：投递两个屏障，或者更好的——让 connectDestroyed 完成后触发信号。

            conn.reset();  // 释放引用，让 muduo unique() == true

            muduo::CountDownLatch latch(1);

            client_.reset();
            // ~TcpClient 投递 A(setCloseCallback) 和 B(forceCloseInLoop)
            // B 内部会 queueInLoop(connectDestroyed) — 在队尾

            // 我们的 latch 在 connectDestroyed 之后投递
            // 但由于 connectDestroyed 是在 B 执行时才追加的，我们这里的 runInLoop
            // 会排在 connectDestroyed 之前（位置 3 vs 位置 4）。
            //
            // 所以我们需要两个 latch：
            muduo::CountDownLatch latch2(1);

            // 方案：在 IO 线程中，先等所有 pending 完成，再等下一轮 pending 完成
            loop->runInLoop([loop, &latch, &latch2]() {
                // 第一轮屏障 — 此时 A, B 已完成，但 connectDestroyed 刚被追加
                // connectDestroyed 会在下一次 doPendingFunctors 执行
                latch.countDown();

                // 在 connectDestroyed 之后执行 latch2.countDown
                loop->queueInLoop([&latch2]() {
                    latch2.countDown();
                });
            });

            // 等待 connectDestroyed 完成
            latch2.wait();
        } else {
            conn.reset();
            client_.reset();
        }

        if (loop) {
            loop->quit();
        }
    }
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
