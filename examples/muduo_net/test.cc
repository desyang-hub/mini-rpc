#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Mutex.h>
#include <atomic>
#include <unistd.h>
#include <thread>

// ========== MyClient.h ==========
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/TcpClient.h"

int main() {
    // ✅ EventLoopThread 内部会启动一个专属线程
    //    并在该线程中创建 EventLoop + 调用 loop()
    muduo::net::EventLoopThread loopThread;
    
    // startLoop() 会阻塞直到子线程中的 EventLoop 创建完毕
    // 返回的指针指向子线程中的 Loop，但只用于传递，不在主线程操作
    muduo::net::EventLoop* loop = loopThread.startLoop();
    
    // TcpClient 可以在主线程构造，但传入的是子线程的 Loop
    // connect() 内部会通过 runInLoop 将实际连接操作投递到子线程
    muduo::net::TcpClient client(loop, 
                                  muduo::net::InetAddress("127.0.0.1", 8082), 
                                  "MyClient");
    client.connect();
    
    // 主线程可以做其他业务逻辑...
    sleep(10);
    
    // 退出时 loopThread 析构会自动 quit + join
    return 0;
}