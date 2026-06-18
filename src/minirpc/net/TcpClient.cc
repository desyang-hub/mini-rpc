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
    // std::cout << "tcpclient create" << std::endl;
    // ✅ EventLoopThread 内部会启动一个专属线程
    //    并在该线程中创建 EventLoop + 调用 loop()
    // muduo::net::EventLoopThread loopThread;

    // startLoop() 会阻塞直到子线程中的 EventLoop 创建完毕
    //    返回的指针指向子线程中的 Loop，但只用于传递，不在主线程操作
    muduo::net::EventLoop *loop = loop_.startLoop();

    // TcpClient 可以在主线程构造，但传入的是子线程的 Loop
    // connect() 内部会通过 runInLoop 将实际连接操作投递到子线程
    client_ = std::make_unique<muduo::net::TcpClient>
    (loop, muduo::net::InetAddress(ep.host.c_str(), ep.port), "MyClient");

    client_->setConnectionCallback(
        [this](const muduo::net::TcpConnectionPtr &conn) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (conn->connected()) {
                    conns_ = conn;       // ✅ 连接建立，保存
                    is_connected_ = true;
                } else {
                    conns_.reset();      // ✅ 连接断开，清除
                    // 可选：触发重连逻辑、通知连接管理器该 Endpoint 不可用等
                    is_connected_ = false;
                }
            }
            condition_.notify_all();
        });
}

TcpClient::~TcpClient() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns_.reset(); // 提前释放连接引用
    }

    if (client_) {
        muduo::net::EventLoop* loop = client_->getLoop();
        client_.reset(); // 触发异步清理

        // 🔑 屏障：确保所有 runInLoop 任务（包括 removeChannel）执行完毕
        if (loop && !loop->isInLoopThread()) {
            muduo::CountDownLatch latch(1);
            loop->runInLoop([&latch]() { latch.countDown(); });
            latch.wait(); // 阻塞直到 IO 线程完成所有待处理任务
        }
    }
    // 此时 loop_ 析构绝对安全，不再有未处理的 Channel
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
        conn = conns_; // 🔑 关键：在锁内安全拷贝
    }

    // conn 是独立的 shared_ptr，不受后续 conns_ 变化影响
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
