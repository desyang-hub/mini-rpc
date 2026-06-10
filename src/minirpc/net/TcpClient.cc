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

namespace minirpc
{

// ====== TcpClient implementation ======

TcpClient::TcpClient(const EndPoint &ep)
    : loop_(), is_connected_(false)
{
    // ✅ EventLoopThread 内部会启动一个专属线程
    //    并在该线程中创建 EventLoop + 调用 loop()
    // muduo::net::EventLoopThread loopThread;

    // startLoop() 会阻塞直到子线程中的 EventLoop 创建完毕
    //    返回的指针指向子线程中的 Loop，但只用于传递，不在主线程操作
    muduo::net::EventLoop *loop = loop_.startLoop();

    // TcpClient 可以在主线程构造，但传入的是子线程的 Loop
    // connect() 内部会通过 runInLoop 将实际连接操作投递到子线程
    clients_ = std::make_unique<muduo::net::TcpClient>(loop, muduo::net::InetAddress(ep.host.c_str(), ep.port), "MyClient");

    clients_->setConnectionCallback([this](const muduo::net::TcpConnectionPtr &conn)
                                    {
            std::lock_guard<std::mutex> lock(mutex_);
            if (conn->connected()) {
                conns_ = conn;       // ✅ 连接建立，保存
                is_connected_ = true;
            } else {
                conns_.reset();      // ✅ 连接断开，清除
                // 可选：触发重连逻辑、通知连接管理器该 Endpoint 不可用等
                is_connected_ = false;
            } });
}

void TcpClient::Start()
{
    clients_->connect();
}

void TcpClient::setMessageCallback(muduo::net::MessageCallback cb)
{
    if (!cb)
    {
        LOG_ERROR("getConnection: messageCallback_ is empty! RpcClient may be destroyed.");
    }
    clients_->setMessageCallback(std::move(cb));
}

void TcpClient::sendRequest(const std::string &req)
{
    return sendRequest(req.c_str(), req.size());
}

void TcpClient::sendRequest(const void *data, size_t len)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!condition_.wait_for(lock, std::chrono::seconds(3),  [this] {
            return is_connected_.load(); 
        })) {
        // 超时逻辑
        throw RpcException("Request Timeouts Error.");
    }

    conns_->getLoop()->runInLoop([this, req = std::string((const char *)data, len)]()
                                 {
            if (conns_->connected()) {
                conns_->send(req);
            } });
}

} // namespace minirpc
