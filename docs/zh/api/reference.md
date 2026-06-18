# API 参考

## RpcClient

单例模式的 RPC 客户端，负责发起远程调用。

### 获取实例

```cpp
minirpc::RpcClient& client = minirpc::RpcClient::GetInstance();
```

### 初始化

```cpp
// 设置 Nacos 地址并初始化服务实例缓存（必须在调用 RPC 之前执行一次）
client.init("127.0.0.1");
```

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `init` | `void init(const std::string& nacosAddr)` | 初始化 Nacos 地址和服务实例缓存 |
| `Call` | `template<class R, class ...Args> R Call(const char* serviceName, const char* name, Args&& ...args)` | 同步调用，返回结果值 |
| `AsyncInvoke` | `std::future<Response> AsyncInvoke(const char* name, const Bytes& bytes, uint64_t request_id)` | 异步调用，返回 future |
| `Invoke` | `template<class R> R Invoke(const char* name, const Bytes& bytes, uint64_t request_id)` | 同步调用已编码的请求 |
| `MessageHandler` | `void MessageHandler(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp t)` | 处理收到的响应消息 |

### Call 方法

```cpp
template<class R, class ...Args>
R Call(const char* serviceName, const char* methodName, Args&& ...args);
```

**参数**:
- `serviceName` — 服务名（如 `UserService.login`）
- `methodName` — 方法名
- `args` — 可变参数，直接传递方法参数

**返回**: 调用结果，类型由模板参数 `R` 指定

**超时**: 默认 200ms，超时抛出 `std::runtime_error`

**异常**: 当状态码非 SUCCESS 时抛出 `RpcException`

---

## RpcServer

服务端的 RPC 处理器，负责注册和管理服务方法。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `Start` | `void Start(int port, const char* name, const char* nacosAddr)` | 启动服务端 |
| `Stop` | `void Stop()` | 停止服务端 |
| `GetInstance` | `static RpcServer& GetInstance()` | 获取单例 |
| `RegisterService` | `template<class R, class F, typename ...Args> void RegisterService(const char* className, const char* name, F&& f)` | 注册服务方法 |
| `Invock` | `bool Invock(const std::string& srvName, const std::string& req, std::string& resp)` | 调用已注册的方法 |
| `addServiceInstance` | `void addServiceInstance(const char* name, const char* groupName, const char* clusterName)` | 添加待注册的服务实例 |

### Start 方法

```cpp
void Start(int port = 8080,
           const char* name = "TcpServer",
           const char* nacosAddr = "127.0.0.1");
```

**参数**:
- `port` — 监听端口
- `name` — 服务器名称
- `nacosAddr` — Nacos 服务注册中心地址

**说明**:
- 基于 muduo EventLoopThread 处理 I/O 事件
- 自动向 Nacos 注册服务
- 请求处理交由 ThreadPool

---

## TcpServer

TCP 服务器，基于 muduo 封装。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造函数 | `TcpServer(int port, const char* name, size_t threadNum)` | 创建服务器 |
| `Start` | `void Start()` | 启动服务器 |
| `Stop` | `void Stop()` | 停止服务器 |
| `setMessageCallback` | `void setMessageCallback(const muduo::net::MessageCallback& cb)` | 设置消息回调 |

---

## ConnectionManager

连接管理器，维护到多个服务端节点的连接池。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `getConnection` | `TcpClientPtr getConnection(const EndPoint& ep)` | 获取到指定端点的连接 |
| `getConnection` | `TcpClientPtr getConnection(const std::vector<EndPoint>& eps)` | 从端点列表中随机选择并获取连接 |
| `setMessageCallback` | `void setMessageCallback(muduo::net::MessageCallback cb)` | 设置消息回调 |
| `recovery` | `void recovery(TcpClientPtr ptr)` | 归还连接到池中 |

---

## ServiceInstanceCache

Nacos 服务实例订阅缓存，基于 `EventListener` 模式实时更新。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `subscribeService` | `void subscribeService(const std::string& serviceName)` | 订阅指定服务 |
| `getInstances` | `std::list<nacos::Instance> getInstances(const std::string& serviceName)` | 获取实例列表（从缓存，无阻塞） |
| `refreshInstance` | `void refreshInstance(const std::string& serviceName)` | 手动刷新实例列表 |
| `shutdown` | `void shutdown()` | 主动销毁，避免静态析构顺序问题 |

---

## Config

TOML 配置文件加载器。

### 数据结构

```cpp
struct Config {
    int port = 8080;
    std::string listen_host = "0.0.0.0";
    std::string registry_address = "127.0.0.1";
    std::string registry_group = "DefaultGroup";
    std::string registry_cluster = "DefaultCluster";
};
```

### 加载配置

```cpp
#include "minirpc/common/Config.h"

// 加载当前目录下的 config.toml
auto config = minirpc::loadConfig();

// 加载指定路径的配置文件
auto config = minirpc::loadConfig("/path/to/config.toml");
```

配置文件不存在或键缺失时，使用默认值。

---

## 序列化

### Serialize

自动根据类型选择序列化方式。

| 方法 | 签名 | 说明 |
|------|------|------|
| `Serialization` | `template<class T> static std::string Serialization(const T& obj)` | 序列化对象 |
| `Deserialization` | `template<class T> static T Deserialization(const std::string& data)` | 从字符串反序列化 |
| `Deserialization` | `template<class T> static T Deserialization(const void* data, size_t len)` | 从字节数据反序列化 |

**自动类型检测**:
- `google::protobuf::Message` 派生类 → Protobuf 序列化
- 其他类型 → JSON 序列化

---

## 宏参考

### RPC_SERVICE_BIND(Class, ...methods)

在服务类中声明要暴露的方法。支持 1~11 个方法。

### RPC_SERVICE_STUB(Class, ...methods)

生成客户端代理类 `Class##_Stub`。支持 1~12 个方法。

---

## Logger

| 方法 | 签名 | 说明 |
|------|------|------|
| `setLevel` | `void setLevel(LogLevel level)` | 设置日志级别 |
| `enable_async_log_write` | `void enable_async_log_write()` | 启用异步日志写入 |

### 日志级别

| 级别 | 宏 | 说明 |
|------|------|------|
| FATAL | `LOG_FATAL(msg)` | 致命错误 |
| ERROR | `LOG_ERROR(msg)` | 错误 |
| INFO | `LOG_INFO(msg)` | 信息 |
| DEBUG | `LOG_DEBUG(msg)` | 调试 |

---

## ThreadPool

| 方法 | 签名 | 说明 |
|------|------|------|
| `enqueue` | `template<typename F, typename... Args> std::future<typename std::result_of<F(Args...)>::type> enqueue(F&& f, Args&&... args)` | 将任务加入线程池 |

---

## Response

RPC 响应结构体。

```cpp
struct Response {
    uint8_t state;      // 状态码: SUCCESS=0, FAILED=1, TIMEOUT=2
    std::string data;   // 响应数据
};
```

---

## EndPoint

网络端点结构体。

```cpp
struct EndPoint {
    std::string host;
    int port;
};
```
