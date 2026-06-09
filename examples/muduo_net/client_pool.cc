#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Mutex.h>
#include <atomic>


using namespace muduo;
using namespace muduo::net;

class TcpClient {
public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, int poolSize)
        : loop_(loop), serverAddr_(serverAddr), index_(0) {
        clients_.reserve(poolSize);
        conns_.resize(poolSize); // 预分配

        for (int i = 0; i < poolSize; ++i) {
            auto client = std::make_unique<TcpClient>(loop, serverAddr, "RpcClient-" + std::to_string(i));
            
            // 捕获 i（注意：必须传值，不能引用！）
            int idx = i;
            client->setConnectionCallback([this, idx](const TcpConnectionPtr& conn) {
                MutexLockGuard lock(mutex_);
                if (conn->connected()) {
                    conns_[idx] = conn;
                } else {
                    conns_[idx].reset();
                }
            });

            // client->setMessageCallback(/* your callback */);
            client->connect();
            clients_.push_back(std::move(client));
        }
    }

    void sendRequest(const std::string& req) {
        int i = index_++;
        size_t idx = i % clients_.size();

        TcpConnectionPtr conn;
        {
            MutexLockGuard lock(mutex_);
            conn = conns_[idx];
        }

        if (conn && conn->connected()) {
            conn->getLoop()->runInLoop([conn, req]() {
                if (conn->connected()) {
                    conn->send(req);
                }
            });
        }
    }

private:
    EventLoop* loop_;
    InetAddress serverAddr_;
    std::vector<std::unique_ptr<TcpClient>> clients_;
    std::vector<TcpConnectionPtr> conns_; // 与 clients_ 一一对应
    mutable MutexLock mutex_;
    std::atomic<int> index_;
};