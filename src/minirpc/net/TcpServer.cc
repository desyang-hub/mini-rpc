#include "minirpc/net/TcpServer.h"
#include <muduo/net/EventLoop.h>

namespace minirpc
{

TcpServer::TcpServer(int port, const char* name, size_t threads)
    : addr_(port)
{
    auto loop = loopThread_.startLoop();
    server_ = std::make_shared<muduo::net::TcpServer>(loop, addr_, name);

    loop->runInLoop([this, threads]() {
        if (threads > 0) server_->setThreadNum(threads);
    });
}

TcpServer::~TcpServer() = default;

void TcpServer::setMessageCallback(const muduo::net::MessageCallback& cb)
{
    server_->setMessageCallback(cb);
}

void TcpServer::Start()
{
    if (server_) {
        server_->getLoop()->runInLoop([this]() { server_->start(); });
    }
}

void TcpServer::Stop()
{
    if (server_) {
        server_->getLoop()->runInLoop([this]() {
            auto loop = server_->getLoop();
            server_.reset();
            loop->quit();
        });
    }
}

} // namespace minirpc
