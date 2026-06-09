#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>

using namespace muduo::net;
using namespace muduo;

int main(int argc, char const *argv[])
{
    muduo::net::InetAddress addr("127.0.0.1", 8080);
    muduo::net::EventLoop loop;
    muduo::net::TcpServer server(&loop, addr, "RpcServer");

    server.setMessageCallback([](const TcpConnectionPtr& conn,
        Buffer* buf,
        Timestamp time){
            conn->send(buf);
    });

    server.start();
    
    loop.loop();

    return 0;
}
