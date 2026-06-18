#include "minirpc/net/TcpClient.h"
#include "minirpc/common/RpcException.h"
#include "minirpc/common/logger.h"
#include "minirpc/net/ConnectionManager.h"

#include <muduo/net/EventLoop.h>

#include <muduo/base/CountDownLatch.h>

namespace minirpc
{

TcpClient::TcpClient(const EndPoint& ep, ConnectionManager* connMgr)
    : ep_(ep), connMgr_(connMgr)
{
    muduo::net::EventLoop* loop = loop_.startLoop();

    client_ = std::make_shared<muduo::net::TcpClient>(
        loop, muduo::net::InetAddress(ep.host.c_str(), ep.port), "RpcClient");

    client_->setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (conn->connected()) {
            conns_ = conn;
            is_connected_ = true;
        } else {
            conns_.reset();
            is_connected_ = false;
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
        if (client_) loop = client_->getLoop();
        conn = conns_;
        conns_.reset();
    }

    if (client_ && loop && !loop->isInLoopThread()) {
        conn.reset();  // Release reference so muduo can destroy connection

        // Dual latch barrier to ensure muduo's cleanup chain completes:
        // pendingFunctors_: setCloseCallback → forceCloseInLoop → latch1 → connectDestroyed → latch2
        muduo::CountDownLatch latch1(1);
        muduo::CountDownLatch latch2(1);

        client_.reset();  // ~TcpClient posts setCloseCallback + forceCloseInLoop

        loop->runInLoop([loop, &latch1, &latch2]() {
            latch1.countDown();  // Barrier after forceCloseInLoop
            loop->queueInLoop([&latch2]() { latch2.countDown(); });  // After connectDestroyed
        });

        latch2.wait();  // Wait for connectDestroyed to complete
    } else if (client_) {
        conn.reset();
        client_.reset();
    }

    if (loop) loop->quit();
}

void TcpClient::Start()
{
    client_->connect();
}

void TcpClient::setMessageCallback(muduo::net::MessageCallback cb)
{
    if (!cb) {
        LOG_ERROR("setMessageCallback called with empty callback");
    }
    client_->setMessageCallback(std::move(cb));
}

void TcpClient::sendRequest(const std::string& data)
{
    sendRequest(data.c_str(), data.size());
}

void TcpClient::sendRequest(const void* data, size_t len)
{
    muduo::net::TcpConnectionPtr conn;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condition_.wait_for(lock, std::chrono::seconds(1),
                [this]{ return conns_ && conns_->connected(); })) {
            throw RpcException("Connection not ready (timeout)");
        }
        conn = conns_;
    }

    auto dataCopy = std::make_shared<std::string>(static_cast<const char*>(data), len);
    conn->getLoop()->runInLoop([conn, dataCopy]() {
        if (conn->connected()) conn->send(*dataCopy);
    });
}

void TcpClient::recovery()
{
    if (connMgr_) {
        connMgr_->recovery(shared_from_this());
    }
}

} // namespace minirpc
