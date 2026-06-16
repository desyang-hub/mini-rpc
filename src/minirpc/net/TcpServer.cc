/**
 * @FilePath     : /mini-rpc/src/minirpc/net/TcpServer.cc
 * @Description  : TcpServer implementation
 * @Author       : desyang
 * @Date         : 2026-06-10 11:48:37
**/
#include "minirpc/net/TcpServer.h"
#include <muduo/net/Buffer.h>

namespace minirpc
{

// ====== TcpServer implementation ======

TcpServer::TcpServer(int port, const char *name, size_t thread_num)
    : loop_(), addr_(port), server_(&loop_, addr_, name)
{
    server_.setThreadNum(thread_num);
}

TcpServer::~TcpServer() = default;

void TcpServer::setMessageCallback(const muduo::net::MessageCallback &cb)
{
    server_.setMessageCallback(cb);
}

void TcpServer::Start()
{
    server_.start();
    loop_.loop();
}

} // namespace minirpc
