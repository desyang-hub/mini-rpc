# 常见问题

## 基础问题

### 什么是 mini-rpc？

mini-rpc 是一个轻量级的 C++ RPC 框架，提供简单易用的 API 来实现远程过程调用。它基于 epoll ET 模式的高性能网络 I/O，支持服务注册与发现（Nacos），适用于分布式系统开发。

### mini-rpc 支持哪些序列化格式？

目前支持 **JSON**（使用 nlohmann/json）。Protobuf 序列化接口已定义但未实现，预留了扩展空间。

### 需要哪些依赖？

- **编译**: CMake >= 3.20, GCC 9+ / Clang 10+, C++17
- **运行时**: libcurl, zlib
- **服务注册中心**: Nacos >= 2.0

## 使用问题

### 如何定义一个新的 RPC 服务？

```cpp
// 1. 在头文件中声明服务类
class MyService {
public:
    int add(int a, int b);
    RPC_SERVICE_BIND(MyService, add);
};

// 2. 在 .cc 文件中注册
RPC_SERVICE_REGISTER(MyService);
```

### 为什么客户端调用时抛出 RpcException？

常见原因：
1. **服务端未启动** — 确认 TcpServer 正在运行
2. **服务名不匹配** — 检查服务名格式 `ClassName.methodName` 是否正确
3. **参数类型不匹配** — 编译期通过 `static_assert` 检查，运行时通过 CRC32 校验
4. **Nacos 服务不可用** — 确认 Nacos 地址和环境变量配置正确

### 如何调试 RPC 调用？

启用 DEBUG 级别日志：

```cpp
minirpc::Logger::GetInstanse().setLevel(minirpc::DEBUG);
```

查看日志输出中的连接建立、消息发送和接收详情。

## 高级问题

### 连接池是如何工作的？

每个服务名对应一个 `RpcConnectionPool`，池中的连接在首次借用时创建（懒加载）。连接池拥有独立的 epoll 事件循环线程，自动管理连接的生命周期（健康检查、断线重连）。

### 可以自定义超时时间吗？

当前 `call()` 方法使用固定的 5 秒超时。如需调整，需修改 `RpcClient::call()` 中的 `wait_for()` 参数。

### 是否支持 Protobuf？

接口已定义但尚未实现。当前使用 JSON 序列化，可通过 `Serialize::SetSerializeType()` 切换格式。Protobuf 支持需要自行实现序列化/反序列化逻辑。

### 如何部署到生产环境？

1. 编译 release 版本：`cmake -DCMAKE_BUILD_TYPE=Release ..`
2. 确保 Nacos 服务可用并配置正确的环境变量
3. 使用 systemd 或 docker 管理服务进程
4. 监控 Nacos 控制台的服务健康状态

## 贡献问题

### 如何贡献代码？

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/xxx`)
3. 提交更改 (`git commit -am 'Add xxx'`)
4. 推送到分支 (`git push origin feature/xxx`)
5. 提交 Pull Request

### 项目使用什么许可证？

[MIT License](https://github.com/desyang-hub/mini-rpc/blob/main/LICENSE)
