#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>

namespace minirpc
{

class TcpServer
{
private:
    muduo::net::EventLoop loop_;
    muduo::net::InetAddress addr_;
    muduo::net::TcpServer server_;

public:
    TcpServer(int port = 8080, const char* name = "TcpServer") : loop_(), addr_(port), server_(&loop_, addr_, name) {

    }
    ~TcpServer() = default;

    /// @brief 用于设置消息回调
    /// @param cb 
    void setMessageCallback(const muduo::net::MessageCallback& cb){
        server_.setMessageCallback(cb);
    }

    void Start() {
        server_.start();
        loop_.loop();
    }
};

    
} // namespace minirpc