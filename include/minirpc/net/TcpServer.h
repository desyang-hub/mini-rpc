/**
 * @FilePath     : /mini-rpc/include/minirpc/net/TcpServer.h
 * @Description  : TcpServer header
 * @Author       : desyang
 * @Date         : 2026-06-10 11:48:37
**/
#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

namespace minirpc
{

class TcpServer
{
public:
    explicit TcpServer(int port = 8080, const char *name = "TcpServer", size_t thread_num = 4);
    ~TcpServer();

    /// @brief 用于设置消息回调
    /// @param cb
    void setMessageCallback(const muduo::net::MessageCallback &cb);

    void Start();

private:
    muduo::net::EventLoop loop_;
    muduo::net::InetAddress addr_;
    muduo::net::TcpServer server_;
};


} // namespace minirpc
