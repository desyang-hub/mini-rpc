# mini-rpc 面试Q&A文档

> 基于对 C++ 轻量级 RPC 框架源码的深入分析

---

## 一、项目概述类

### Q1: 请介绍一下 mini-rpc 这个项目的整体架构

**回答要点：**

mini-rpc 是一个基于 C++17、采用 Reactor 模式的轻量级 RPC 框架，整体采用分层架构设计：

| 层级 | 模块 | 职责 |
|------|------|------|
| 网络层 | `net/` | Epoll 事件循环、TcpServer/TcpClient、Channel 封装 |
| 核心层 | `core/` | RpcClient、RpcServer、连接池管理 |
| 协议层 | `protocol/` | Protobuf/JSON 序列化、编解码器 |
| 公共层 | `common/` | 线程池、日志、RingBuffer、工具函数 |

**核心架构图：**

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                          │
│  ┌─────────────────┐         ┌─────────────────┐            │
│  │ protoc → .h/.cc │         │ Python CodeGen  │            │
│  └────────┬────────┘         └────────┬────────┘            │
│           │                           │                     │
└───────────┼───────────────────────────┼─────────────────────┘
            │                           │
┌───────────┼───────────────────────────┼─────────────────────┐
│   Protocol Layer                    Service Layer           │
│  ┌──────────────┐                   ┌──────────────┐        │
│  │ Encoder/     │                   │  RpcClient   │        │
│  │ Decoder      │◄─────────────────►│  RpcServer   │        │
│  └──────┬───────┘                   └──────┬───────┘        │
│         │                                  │                │
└─────────┼──────────────────────────────────┼────────────────┘
          │                       ┌──────────┼───────────┐ 
┌─────────┼───────────────────────┼──────────┼───────────┼───┐
│         │   Core Layer          │          │           │   │
│  ┌──────┴──────┐   ┌────────────┴───┐   ┌──┴────────┐  │   │
│  │RpcConnection│   │RpcConnPool     │   │TcpSrv/Poll│  │   │
│  └──────┬──────┘   └────────────┬───┘   └────┬──────┘  │   │
│         │                       │            │         │   │
└─────────┼───────────────────────┼────────────┼─────────┼───┘
          │  EventLoop/Poller     │  ThreadPool  │           │
          │  (Channel)            │  (Task)      │           │
          └───────────────────────┴──────────────┘           │
                           │                                 │
                    ┌──────┴──────┐                          │
                    │  Epoll ET   │                          │
                    │  /dev/epoll │                          │
                    └─────────────┘                          │
                            │                                │
                    ┌───────┴───────┐                        │
                    │  Nacos SDK    │                        │
                    │  Service      │                        │
                    │  Discovery    │                        │
                    └───────────────┘                        │
                                                             │
                                                     ┌───────┴───────┐
                                                     │  TCP Socket   │
                                                     └───────────────┘
```

## 二、网络 I/O 模型类

### Q2: 为什么选择 epoll ET 模式？ET 和 LT 模式有什么区别？

**回答要点：**

**ET (Edge-Triggered) 模式特点：**
- **触发条件**：文件描述符状态变化时才通知一次（例如：从不可读变为可读）
- **行为**：如果第一次处理时没有读完所有数据，后续不会再次通知
- **要求**：必须使用非阻塞 I/O，每次事件到来时需要循环读取直到返回 EAGAIN/EWOULDBLOCK

**LT (Level-Triggered) 模式特点：**
- **触发条件**：只要文件描述符处于就绪状态，每次 epoll_wait 都会通知
- **行为**：即使上次只读了一部分数据，下次 epoll_wait 仍会通知
- **容错性更好**：不容易遗漏事件

**选择 ET 模式的原因：**

```cpp
// src/minirpc/net/EpollPoller.cc
ev.events = EPOLLIN | EPOLLET; // ET 触发
```

1. **减少事件通知次数**：ET 模式只需一次通知就能处理完所有数据，降低系统调用开销
2. **提高吞吐量**：适合高并发场景，减少 epoll_wait 被唤醒的次数
3. **符合本项目的异步架构**：配合线程池处理客户端请求，每个连接的事件循环只负责分发

**ET 模式的挑战**：
```cpp
// TcpServer::ClienHandler - ET 模式必须循环读完
ET模式：一次处理完所有数据
while (true) {
    int len = c->readMsg();
    if (len < 0 || len == 0) break;
    // 处理数据...
}
```

### Q3: epoll 的 wait 和 ctl 有什么区别？epollfd 的设计思路是什么？

**回答要点：**

本项目有**两个 epollfd**：

1. **TcpServer 的 epollfd** — 监听所有客户端连接（accept + 分发处理）
2. **RpcConnectionPool 的 epollfd** — 管理客户端到服务端的连接池

```cpp
// TcpServer::loop() - 服务端 epoll
epollfd_ = epoll_create1(EPOLL_CLOEXEC);  // 创建
epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &ev);  // server socket
// ...
epoll_wait(epollfd_, events.data(), events.size(), -1);  // 等待

// RpcConnectionPool::addConnectionListener() - 连接池 epoll
epollfd_ = epoll_create1(EPOLL_CLOEXEC);  // 创建（首次）
epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &ev);  // 添加客户端连接
```

**CLOEXEC 标志**：exec 时自动关闭 fd，防止文件描述符泄漏到子进程

### Q4: Channel-Poller-EventLoop 的三层结构是怎么设计的？

**回答要点：**

这是经典的 **Reactor 模式**实现：

| 组件 | 职责 | 类比 |
|------|------|------|
| Channel | 代表一个 fd 对应的事件集合，携带回调函数 | "插座"—承载事件和数据 |
| Poller | 底层 I/O 多路复用器，抽象 epoll/kqueue | "总闸"—监听所有插座 |
| EventLoop | 运行一个事件循环，调用 Poller 并派发 | "调度员"—分发包处理器 |

**交互流程：**

```
epoll_wait()           poller::poll()         EventLoop::loop()
     │                        │                        │
     ├── 等待事件 ───────────▶│                        │
     │                        ├── 填充事件列表 ──────▶ │
     │                        │                        ├── 遍历 Channel
     │                        │                        │  │
     │                        │                        │  ├── 调用回调类
```

**关键设计**：Channel 持有 `tie_`（weak_ptr），确保回调执行时对象未销毁：

```cpp
// Channel::handleEvent
if (tied_) {
    std::shared_ptr<void> guard = tie_.lock();
    if (guard) {
        handleEventWithGuard(timeStamp);
    }
}
```

---

## 三、连接池管理类

### Q5: 连接池的生命周期是如何管理的？

**回答要点：**

**连接池架构：**

```
connection_pools_[server_name@group_name] 
    └── RpcConnectionPool
            ├── epollfd + loop_thread (事件循环)
            ├── threadPool_ (任务分发)  
            └── connection_que_ (连接队列)
```

**生命周期：**

```
创建:  TcpConnectionPoolFactory::getConnectionPool()
            │
            ├── 缓存 key = "server_name@group_name"
            ├── 若不存在 → 创建 RpcConnectionPool (启动 loop_thread)
            └── 放入 connection_pools_ 缓存

借用:  RpcConnectionPool::getConnection()
            │
            ├── 遍历连接队列
            ├── 检查 health → 健康则借出
            ├── 不健康则销毁
            └── 空 → 创建新连接 (connect + addConnectionListener)

归还:  RpcConnectionPool::returnConnection()
            │
            └── 放回队列 (不关闭 socket)

销毁:  ~RpcConnectionPool() + ~RpcClient()
            │
            ├── pool_running_ = false
            ├── join loop_thread
            ├── close epollfd
            └── 连接排队销毁
```

### Q6: 连接池如何实现健康检查？

**回答要点：**

```cpp
// RpcConnectionPool::getConnection()
for (int i = 0; i < connection_que_.size(); ++i) {
    if (connection_que_.front()->isHealthy()) {
        conPtr = std::move(connection_que_.front());
        connection_que_.pop();
        break;
    }
    // 销毁不健康的连接
    connection_que_.pop();
}
```

**健康检查策略：**
1. **连接尝试阶段**：connect() 失败 → 标记 unhealthy
2. **读写错误阶段**：recv 返回 -1 或 0 → 标记 unhealthy
3. **借连接时清理**：遍历队列时跳过 unhealthy 连接并销毁

### Q7: 监听通知（notifier）的作用是什么？为什么需要 eventfd？

**回答要点：**

在 EventLoop 中，**同一线程内**需要一种机制来唤醒 epoll_wait：

```cpp
// 使用 eventfd 作为通知文件描述符
int eventfd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
```

**作用：**
1. Waker/Notifier 线程可以通过 write(eventfd, &num, 8) 唤醒 EventLoop 的 epoll_wait
2. epoll_wait 收到通知后会返回，EventLoop 调用 handler_->handleEvents() 执行回调清理
3. 避免空等待（epoll_wait 超时反复轮询浪费 CPU）

---

## 四、并发控制类

### Q8: 为什么在 RpcServer 中要使用 shared_mutex？

**回答要点：**

```cpp
class RpcServer {
    mutable std::shared_mutex handlers_mutex_;
    std::unordered_map<std::string, RpcHandler> handlers_;
```

**shared_mutex 的作用：**
- **写锁**（exclusive_lock）：registerService 时独占访问，确保线程安全
- **读锁**（shared_lock）：call 方法可同时读，不阻塞其他线程的调用

**为什么不用普通 mutex？**

```cpp
// call 方法使用 shared_lock - 允许多线程并发调用
std::shared_lock<std::shared_mutex> lock(handlers_mutex_);
it = handlers_.find(name);

// registerService 使用 unique_lock - 独占写
std::unique_lock<std::shared_mutex> lock(handlers_mutex_);
services_.push_back(className);
```

**权衡：** 简单场景中可以改用 mutex，但 shared_mutex 对于读多写少的场景性能更好

### Q9: 线程池的设计思路是什么？

**回答要点：**

```cpp
// ThreadPool 核心设计
class ThreadPool {
    std::queue<TaskHandler> queue_;
    std::vector<std::thread> workers_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
};
```

**关键技术点：**

1. **条件变量等待**：`condition_.wait(lock, predicate)` 避免忙等待
2. **packaged_task + future**：任务执行结果可通过 future 获取
3. **RAII 销毁**：析构时 set is_running_=false + notify_all，确保线程退出

**使用场景：**

| 位置 | 用途 | 线程数 |
|------|------|--------|
| TcpServer | ClienHandler 分发 | 默认4 |
| RpcConnectionPool | 连接池消息处理 | 默认4 |

### Q10: Lambda 捕获在多线程环境中需要注意什么？

**回答要点：**

本项目的典型用法：

```cpp
// RpcConnectionPool::loop() - 捕获 this 指针
threadPool_.enqueue(message_handler_, c);

// RpcClient::messageHandler() - 捕获 this
auto it = promiseMap_.find(header.request_id);
it->second.set_value({header.code, body});
```

**注意点：**
1. **生命周期安全**：确保 lambda 执行时对象未被销毁
2. **按值捕获 vs 引用捕获**：引用捕获 `this` 需关注对象是否存在
3. **原子操作**：shared_mutex + atomic<bool> 用于线程安全的数据共享

---

## 五、协议序列化类

### Q11: 协议的序列化方式是怎样的？

**回答要点：**

```
┌──────────────────────────────────────────────────────┐
│                    协议帧                              │
├──────────┬──────────┬──────────────┬────────────────┤
│ Header   │  Type    │ RequestID    │ Body (var len) │
│ (8 bytes)│ (1 byte) │ (8 bytes)    │                │
├──────────┴──────────┴──────────────┴────────────────┤
│  Magic: 0x12345678   │  Version: 0x01               │
└─────────────────────────────────────────────────────┘
```

**序列化流程：**
```
RpcClient.send() → Encoder::Encode() → protobuf/json → TCP Socket
RpcServer.receive() → Decoder::Decode() → RpcServer::call() → Protobuf/json
```

---

## 六、服务发现类

### Q12: Nacos 集成做了哪些功能？

**回答要点：**

```cpp
// TcpServer::lunch_service_register() - 服务注册
void register_services(const std::vector<std::string>& services, ...) {
    auto g_namingSvc = factory->CreateNamingService();
    
    Instance instance;
    instance.clusterName = "DefaultCluster";
    instance.ip = host;
    instance.port = server_port;
    instance.ephemeral = true;  // 临时实例
    
    for (const std::string& serviceName : services) {
        g_namingSvc->registerInstance(serviceName, instance);
    }
}
```

**注册的服务发现功能：**
1. **服务注册**：启动时将服务注册到 Nacos 服务中心
2. **心跳保活**：Nacos SDK 自动管理心跳
3. **服务发现**：客户端通过 Nacos 查询服务 IP

---

## 七、代码生成类

### Q13: 代码生成工具链是怎么工作的？

**回答要点：**

```python
# Python 代码生成器（伪代码）
 protoc --proto_path=protos message.proto --python_out=src_generated

# 生成的代码示例：
 class TestService_Stub(object):
     def __init__(self, client):  # 传入 RpcClient 实例
         self.client = client
     
     def add(self, a, b):  # 自动生成调用
         req = TestRequest(a=a, b=b)
         body = serialize(TOOL_CALL, req)
         data = self.client.send(body, service_name="TestService", group_name="DEFAULT_GROUP")
         response = deserialize_add_response(data)
         return response
```

**macro 扩展：**
```cpp
// 服务端：注册服务到 map
RPC_SERVICE_REGISTER(TestService);

// 客户端：生成 Stub 调用壳
RPC_SERVICE_STUB(TestService, add, sub);
```

---

## 八、高级话题类

### Q14: 设计一个高性能 RPC 框架需要考虑哪些维度？

**回答要点：**

| 维度 | 考量点 | 本项目实现 |
|------|--------|-----------|
| I/O 模型 | ET vs LT、零拷贝 | ET 模式 |
| 并发模型 | Reactor、Invoker、线程池调度 | EventLoop + Reactor |
| 序列化 | Protobuf、FlatBuffers、自研协议 | Protobuf/JSON |
| 连接管理 | 主动探测、健康检查、池化 | 连接池 + isHealthy() |
| 服务治理 | 发现、注册、负载均衡、限流 | Nacos |
| 延迟优化 | NIO、sendfile、io_uring | 当前 epoll… |

### Q15: 当前架构有哪些不足之处？如果要改进，你会怎么做？

**回答要点：**

| 不足 | 改进方向 |
|------|----------|
| 连接池缺少超时回收 | 增加空闲超时自动清理 |
| 缺少一致性哈希 | 当前使用随机选择 |
| Proto<b /json 序列化互斥 | 支持混合选择 |
| eventfd 唤醒不够灵活 | 支持条件唤醒 |
| 错误重试缺失 | 增加重试策略 |
| et 模式的高并发吞吐受限 | 考虑 io_uring |
