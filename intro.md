# Mini-RPC 架构介绍

## 概述

Mini-RPC 是一个轻量级 C++17 RPC 框架（v3.2.0），提供基于宏的服务绑定/存根生成、高性能异步 I/O（基于 muduo 的 epoll ET 反应堆模型）以及 Nacos 服务发现集成。采用 MIT 开源协议。

## 模块划分

项目分为四个核心库：

```
include/minirpc/   ← 公共头文件
├── common/        ← 工具层
├── protocol/      ← 协议层（仅头文件）
├── net/           ← 网络传输层
└── core/          ← RPC 框架层

src/minirpc/       ← 对应实现文件
```

### 1. Common — 工具层

提供基础类型与通用工具：

| 组件 | 功能 |
|------|------|
| `function_traits.h` | **核心** — 编译期函数内省（返回类型、参数元组、参数个数），支持 lambda、成员函数指针、自由函数 |
| `tuple_helper.h` | `rpc_apply()` — 将 `std::tuple` 解包为变参函数调用 |
| `ThreadPool.h` | 异步任务线程池 |
| `BlockedQueue.h` | 线程安全阻塞队列 |
| `Config.h` | TOML 格式配置加载（端口、主机、注册中心地址等） |
| `logger.h` | 日志设施（LOG_INFO / LOG_ERROR / LOG_FATAL） |
| `RpcException.h` | RPC 异常类 |
| `Response.h` | 统一响应结构 `{ state, data }` |
| `Buffer.h` | 二进制缓冲区封装 |
| `utils.h` | CRC32 校验、超时获取等工具函数 |

### 2. Protocol — 协议层

定义二进制 RPC wire 格式与序列化：

**27 字节消息头（packed struct）：**

```
+--------+----------+-------+---------+----------+-----------+----------+----------+------------+--------+
| Magic  | Version  | Type  | SerType | Compress | RequestID | BodyLen  | Checksum | SrvNameLen | Code   |
| 2 byte | 1 byte   | 1 byte| 1 byte  | 1 byte   | 4 bytes   | 4 bytes  | 4 bytes  | 4 bytes    | 1 byte |
+--------+----------+-------+---------+----------+-----------+----------+----------+------------+--------+
```

- **Magic**: `0x5250` ("RP")
- **消息类型**: `MSG_REQUEST` / `MSG_RESPONSE` / `MSG_HEARTBEAT`
- **序列化类型**: JSON / Protobuf

**完整数据包格式：** `[Header(27B) | ServiceName | Body | CRC32(4B)]`

| 组件 | 功能 |
|------|------|
| `Encoder.h` | 构建数据包（请求、成功响应、错误响应） |
| `Decoder.h` | 解析接收数据，校验 magic + CRC + body 大小（最大 64MB） |
| `Serialize.h` | **自动检测**序列化后端 — 类型继承 `protobuf::Message` 则用 Protobuf，否则回退 JSON |
| `JsonSerialize.h` | JSON 序列化（nlohmann/json） |
| `ProtobufSerialize.h` | Protobuf 序列化 |

### 3. Net — 网络传输层

基于 muduo 的网络封装：

| 组件 | 功能 |
|------|------|
| `TcpServer` | 封装 `muduo::net::TcpServer`，运行在独立 EventLoopThread 中，可配置端口和线程数 |
| `TcpClient` | 封装 `muduo::net::TcpClient`，管理连接状态、发送请求、连接恢复 |
| `ConnectionManager` | **连接池** — 管理多个 EndPoint 的 TcpClient 集合，`getConnection()` 随机负载均衡，支持连接复用 |

### 4. Core — RPC 框架层

RPC 框架的核心编排逻辑：

| 组件 | 功能 |
|------|------|
| `RpcServer` | **服务端单例**，维护 handlers map (`string -> handler`)，`RegisterService<R>()` 利用 function_traits 提取参数类型并在运行时序列化/反序列化，后台线程向 Nacos 注册服务 |
| `RpcClient` | **客户端单例**，`Call<R>()` 序列化参数 → 编码请求 → 通过连接池发送 → 等待 future（200ms 超时）→ 反序列化返回值 |
| `ServiceInstanceCache` | 订阅 Nacos 命名服务，实时获取服务实例列表更新（通过 `ServiceChangeListener`） |
| `PendingRequest` | 记录在途请求 `{ conn, promise }` |
| `rpc_service_bind.h` | **服务端宏** — `RPC_SERVICE_BIND(Class, M1, M2...)` 生成 GetInstance()、Init()，`RPC_SERVICE_REGISTER(Class)` 创建静态初始化器，支持最多 11 个方法 |
| `rpc_service_stub.h` | **客户端宏** — `RPC_SERVICE_STUB(Class, M1, M2...)` 生成 `Class_Stub` 类，每个方法调用 `RpcClient::Call<ReturnType>(...)`，支持最多 12 个方法 |

## 数据流

### 服务端流程

```
RpcServer::Start(port)
    → 创建 TcpServer，开始监听
    → ServiceRegisterWorker 线程向 Nacos 注册服务

[收到数据]
    → muduo 回调触发 MessageHandler
    → Decoder::check() 校验数据包
    → Decoder::decode() 提取 serviceName + body
    → handlers_[srvName + "." + methodName] 查找处理器
    → 处理器反序列化参数 → 调用绑定的业务方法 → 序列化返回值
    → Encoder::successResponse() 构建响应包 → 发送回客户端
```

### 客户端流程

```
RpcClient::init(nacosAddr)
    → 创建 ServiceInstanceCache，连接 Nacos

[Stub 方法调用]
    → RpcClient::Call<R>(serviceName, method, args...)
    → asyncInvoke() 查询 Nacos 获取服务实例
    → ConnectionManager 选择连接（随机负载均衡）
    → 参数序列化 → 编码为请求包 → 发送
    → MessageHandler() 收到响应 → 解码 → 兑现 promise
    → invoke<R>() 等待 future（200ms 超时）→ 反序列化返回值
```

## 外部依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| **muduo** | v2.0.2 | Reactor 模型异步网络 I/O（epoll ET） |
| **nlohmann/json** | v3.11.2 | JSON 序列化 |
| **nacos-sdk-cpp** | v1.1.3 | Nacos 服务注册中心客户端 |
| **Protobuf** | — | 可选序列化后端 |
| **toml++** | — | TOML 配置文件解析 |
| **Google Test** | v1.14.0 | 测试框架（可选） |

## 使用示例

```cpp
// 1. 定义服务接口
class UserService {
public:
    Response Login(const std::string& user, const std::string& pwd);
};

// 2. 服务端注册
RPC_SERVICE_BIND(UserService, Login);
RPC_SERVICE_REGISTER(UserService);

// 启动服务器
RpcServer::GetInstance().Start(Config::GetInstance().get_port());

// 3. 客户端调用
RPC_SERVICE_STUB(UserService, Login);

RpcClient::GetInstance().init("127.0.0.1:8848");
auto stub = UserService_Stub::New();
auto resp = stub->Login("alice", "secret");
```

## 项目结构

```
mini-rpc/
  CMakeLists.txt           # 根构建配置（FetchContent 依赖、4 个库、主可执行文件）
  src/main.cc              # 入口（打印版本号）
  include/minirpc/         # 所有公共头文件
  src/minirpc/             # 所有实现文件
  examples/                # 3 个示例项目
  ├── basic_json/          # JSON 序列化示例
  ├── basic_protobuf/      # Protobuf 序列化示例
  └── protobuf_bench/      # 性能基准测试
  tests/                   # GTest 测试套件
  cmake/                   # version.h.in 模板
  deploy/                  # 部署脚本
  scripts/                 # 构建/工具脚本
```
