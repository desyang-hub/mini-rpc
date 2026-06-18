### 注意这个初始化，必须要memset，或者是{}进行默认初始化，否则会发生意想不到的错误，比如connect一直卡住
sockaddr_in server_addr

### RpcClient
socket 需要设置为非阻塞，否则epoll_wait将一直阻塞


### 使用close(sockfd) close(epollfd) 无法触发epoll_wait
epoll_wait 在没有事件时永久阻塞，且无法通过关闭 fd 或修改内存标志可靠地唤醒

### unique_ptr 自定义删除器
可以用于管理对象的生命周期，比如对于连接对象，当所有权交由池，管理的时候，连接销毁时，应当归还池子，而不是直接关闭连接，因此需要自定义删除器，在删除时归还连接池


### 遇到一个坑
对于muduo库的回调，必须要多次处理缓冲区中的完整包，因为消息回调过程可能缓冲区中已经

### muduo库的回调，必须要多次处理缓冲区中的完整包，因为消息回调过程可能缓冲区中已经
这是rpc测试工具，对于100个线程，每个线程1000次请求的结果为
================ Benchmark Results ================
Total Time:      31.94 s
Success:         100000
Failed:          0
QPS:             3130.74
Avg Latency:     31.83 ms
===================================================


### 遇到一个大坑，对于muduo库的使用，如下，EventLoopThread必须要在std::shared_ptr<muduo::net::TcpClient> client_的后面声明，确保析构顺序，否则会发生竞争导致未知异常
EndPoint ep_;
ConnectionManager* connMgr_;
muduo::net::MessageCallback messageCallBack_;
std::shared_ptr<muduo::net::TcpClient> client_;
muduo::net::EventLoopThread loop_;
muduo::net::TcpConnectionPtr conns_; // 与 clients_ 一一对应

std::mutex mutex_;
std::condition_variable condition_;
std::atomic<bool> is_connected_;


================ MiniRPC Benchmark ================
Target: 127.0.0.1:8083
Concurrency: 1000, Total Requests: 100000
===================================================

================ Benchmark Results ================
Total Time:      3.77 s
Success:         100000
Failed:          0
QPS:             26502.20
Avg Latency:     34.55 ms
===================================================



### 使用nacos服务订阅，进一步提升rpc性能，将rpc服务注册到nacos，客户端订阅服务，获取服务地址，然后进行rpc调用，性能提升明显，但是需要额外维护nacos服务，且需要额外配置nacos服务，因此暂时不使用，后续可以考虑使用etcd

================ MiniRPC Benchmark ================
Target: 127.0.0.1:8083
Concurrency: 1000, Total Requests: 100000
===================================================

================ Benchmark Results ================
Total Time:      3.71 s
Success:         100000
Failed:          0
QPS:             26932.72
Avg Latency:     34.35 ms
===================================================

================ MiniRPC Benchmark ================
Target: 127.0.0.1:8083
Concurrency: 100, Total Requests: 100000
===================================================

================ Benchmark Results ================
Total Time:      3.42 s
Success:         100000
Failed:          0
QPS:             29226.36
Avg Latency:     3.35 ms
===================================================


================ MiniRPC Benchmark ================
Target: 127.0.0.1:8083
Concurrency: 500, Total Requests: 250000
===================================================

================ Benchmark Results ================
Total Time:      8.89 s
Success:         250000
Failed:          0
QPS:             28113.90
Avg Latency:     17.20 ms
===================================================