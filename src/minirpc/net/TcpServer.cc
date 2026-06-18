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
    : loopThread(), addr_(port)
{
    auto loop_ = loopThread.startLoop();
    server_ = std::make_shared<muduo::net::TcpServer>(loop_, addr_, name);

    // 确保线程数设置正确
    // 在 IO 线程中安全地创建和配置 TcpServer
    loop_->runInLoop([this, thread_num]() {
        if (thread_num > 0) {
            server_->setThreadNum(thread_num);
        }
    });
}

TcpServer::~TcpServer() = default;

void TcpServer::setMessageCallback(const muduo::net::MessageCallback &cb)
{
    server_->setMessageCallback(cb);
}

void TcpServer::Start()
{
    if (server_) {
        // 在子线程内启动
        server_->getLoop()->runInLoop([this]{
            server_->start();
        });
    }
}

void TcpServer::Stop() {
    
    if (server_) {
        // 【核心修复】将销毁工作投递到 IO 线程！
        server_->getLoop()->runInLoop([this]() {
            // 在 IO 线程中销毁，assertInLoopThread() 检查完美通过！
            auto loop = server_->getLoop();
            server_.reset(); 
            // 销毁完毕后，安全退出事件循环
            loop->quit();
        });
    }
}

} // namespace minirpc
