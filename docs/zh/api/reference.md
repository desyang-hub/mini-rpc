# API 参考

## RpcClient

单例模式的 RPC 客户端，负责发起远程调用。

### 获取实例

```cpp
minirpc::RpcClient& client = minirpc::RpcClient::GetInstance();
```

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `call` | `template<T> bool call(const std::string& name, const args_tuple& args, T& ret)` | 同步调用，返回状态码 |
| `call` | `template<> bool call(const std::string& name, const args_tuple& args)` | 同步调用（void 返回类型），返回状态码 |
| `async_call` | `template<typename F, typename... Args> std::future<typename std::result_of<F(Args...)>::type> async_call(F&& f, Args&&... args)` | 异步调用，包装在 std::async 中 |
| `messageHandler` | `void messageHandler(IConnection* conn)` | 处理收到的响应消息 |

### call 方法签名

```cpp
// 有返回值
template<typename Ret, typename... Args>
bool call(const std::string& serviceName, std::tuple<Args...> args, Ret& ret);

// void 返回类型
template<typename... Args>
bool call(const std::string& serviceName, std::tuple<Args...> args);
```

**参数**:
- `serviceName` — 服务名，格式 `ClassName.methodName`（如 `UserService.login`）
- `args` — 参数元组，类型必须与服务方法签名匹配
- `ret` — 输出参数，接收返回值

**返回**: `true` 调用成功，`false` 调用失败（超时/连接断开）

**异常**: 当状态码非 SUCCESS 时抛出 `RpcException`

---

## RpcServer

服务端的 RPC 处理器，负责注册和管理服务方法。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `Bind` | `void Bind(const std::string& name, RpcHandler handler)` | 绑定方法到服务名 |
| `Call` | `bool Call(const std::string& name, const std::string& body, std::string& res)` | 调用已注册的方法 |
| `RegisterService` | `void RegisterService(const std::string& serviceName)` | 注册服务名到 Nacos |

### Bind 方法

```cpp
void Bind(const std::string& name, RpcHandler handler);
```

**参数**:
- `name` — 服务名（如 `UserService.login`）
- `handler` — 处理函数，类型为 `std::function<std::string(const std::tuple<Args...>&)>`

### Call 方法

```cpp
bool Call(const std::string& name, const std::string& body, std::string& res);
```

**参数**:
- `name` — 服务名
- `body` — 请求体（JSON 格式的参数序列化）
- `res` — 输出参数，接收响应体

**返回**: `true` 调用成功，`false` 方法未找到

---

## TcpServer

TCP 服务器，管理监听 socket 和客户端连接。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `serve` | `void serve(int port)` | 在指定端口启动服务器 |

### serve 方法

```cpp
void serve(int port);
```

**参数**:
- `port` — 监听端口

**说明**:
- 使用 epoll ET 模式处理 I/O 事件
- 自动向 Nacos 注册服务
- 主线程运行事件循环，请求处理交由 ThreadPool

---

## IConnection

连接接口，定义了连接操作的抽象基类。

### 核心方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `send` | `virtual bool send(const std::string& data) = 0` | 发送数据 |
| `readMsg` | `virtual int readMsg() = 0` | 读取消息，返回读取字节数 |
| `decode` | `virtual bool decode(ProtocolHeader& header, std::string& body) = 0` | 解码消息 |
| `isHealthy` | `virtual bool isHealthy() const = 0` | 检查连接健康状态 |
| `fd` | `virtual int fd() const = 0` | 获取文件描述符 |
| `close` | `virtual void close() = 0` | 关闭连接 |
| `targetAddress` | `virtual std::string targetAddress() const = 0` | 获取目标地址 |
| `processConnection` | `static bool processConnection(IConnection* conn, ...)` | ET 模式下的批量消息处理 |

---

## 序列化

### Serialize

| 方法 | 签名 | 说明 |
|------|------|------|
| `Serialization` | `static std::string Serialization(const std::tuple<Args...>& args)` | 序列化参数为 JSON 字符串 |
| `Deserialization` | `static T Deserialization(const std::string& body)` | 从 JSON 字符串反序列化为 T 类型 |
| `SetSerializeType` | `static void SetSerializeType(SerializeType type)` | 设置序列化类型 |

---

## 宏参考

### RPC_SERVICE_BIND(Class, ...methods)

在服务类中声明要暴露的方法。

### RPC_SERVICE_STUB(Class, ...methods)

生成客户端代理类 `Class##_Stub`。

### RPC_SERVICE_REGISTER(Class)

在 `.cc` 文件中注册服务。

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
