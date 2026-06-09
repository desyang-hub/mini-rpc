#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/Callbacks.h>
#include <atomic>

namespace minirpc
{

// 后续操作是通过配置文件来进行远程服务注册中心查询可用实例，并进一步获取实例地址，进行连接

class TcpClient {

public:
    TcpClient(const muduo::net::InetAddress& serverAddr, const muduo::net::MessageCallback& cb, int poolSize = 1)
        : loop_(), serverAddr_(serverAddr), index_(0), messageCallBack_(cb) {
        clients_.reserve(poolSize);
        conns_.resize(poolSize); // 预分配

        for (int i = 0; i < poolSize; ++i) {
            auto client = std::make_unique<muduo::net::TcpClient>(&loop_, serverAddr, "RpcClient-" + std::to_string(i));
            
            // 捕获 i（注意：必须传值，不能引用！）
            int idx = i;
            client->setConnectionCallback([this, idx](const muduo::net::TcpConnectionPtr& conn) {
                muduo::MutexLockGuard lock(mutex_);
                if (conn->connected()) {
                    conns_[idx] = conn;
                } else {
                    conns_[idx].reset();
                }
            });

            if (messageCallBack_)
                client->setMessageCallback(messageCallBack_);

            // client->setMessageCallback(/* your callback */);
            client->connect();
            clients_.push_back(std::move(client));
        }
    }

    void Start() {
        loop_.loop();
    }

    void sendRequest(const std::string& req) {
        return sendRequest(req.c_str(), req.size());
    }


    void sendRequest(const void* data, size_t len) {
        int i = index_++;
        size_t idx = i % clients_.size();

        muduo::net::TcpConnectionPtr conn;
        {
            muduo::MutexLockGuard lock(mutex_);
            conn = conns_[idx];
        }

        if (conn && conn->connected()) {
            conn->getLoop()->runInLoop([conn, req = std::string((const char*)data, len)]() {
                if (conn->connected()) {
                    conn->send(req);
                }
            });
        }
    }

private:
    muduo::net::EventLoop loop_;
    muduo::net::InetAddress serverAddr_;
    muduo::net::MessageCallback messageCallBack_;
    std::vector<std::unique_ptr<muduo::net::TcpClient>> clients_;
    std::vector<muduo::net::TcpConnectionPtr> conns_; // 与 clients_ 一一对应
    mutable muduo::MutexLock mutex_;
    std::atomic<int> index_;
};

} // namespace minirpc