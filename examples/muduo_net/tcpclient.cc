#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <unistd.h>
#include <thread>

using namespace muduo;
using namespace muduo::net;

int main(int argc, char const *argv[])
{
    
    muduo::net::InetAddress addr("127.0.0.1", 8080);
    muduo::net::EventLoop loop;
    muduo::net::TcpClient client(&loop, addr, "client");

    client.connect();

    client.setConnectionCallback([](const TcpConnectionPtr& conn){
        if (conn->connected()) {
            std::thread th([conn]{
                while (true) {
                    // send hello per seconds
                    conn->send("hello");
                    sleep(1);
                }
            });
            th.detach();
            
        } else {
            LOG_WARN << "connection disconnect.";
            // 退出循环
            conn->getLoop()->quit();
        }
    });

    client.setMessageCallback([](const TcpConnectionPtr& conn,
        Buffer* buf,
        Timestamp t){
            LOG_INFO << "RECV: " << buf->retrieveAllAsString();
    });

    loop.loop();

    return 0;
}
