#pragma once

#include <memory>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>

namespace minirpc
{

class TcpServer
{
public:
    explicit TcpServer(int port = 8080, const char* name = "TcpServer", size_t threads = 8);
    ~TcpServer();

    void setMessageCallback(const muduo::net::MessageCallback& cb);
    void Start();
    void Stop();

private:
    muduo::net::InetAddress addr_;
    std::shared_ptr<muduo::net::TcpServer> server_;
    muduo::net::EventLoopThread loopThread_;
};

} // namespace minirpc
