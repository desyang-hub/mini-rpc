# 常见问题

## 基础问题

### 什么是 mini-rpc？

mini-rpc 是一个轻量级的 C++ RPC 框架，提供简单易用的 API 来实现远程过程调用。它基于 muduo 网络库和 Nacos 服务注册中心，适用于分布式系统开发。

### mini-rpc 支持哪些序列化格式？

框架根据参数类型**自动选择**序列化方式：

- **JSON** (nlohmann/json) — 适用于基本类型、`std::string`、`std::tuple`、`std::vector` 等
- **Protobuf** — 适用于继承自 `google::protobuf::Message` 的类型

### 需要哪些依赖？

- **编译**: CMake >= 3.20, GCC 9+ / Clang 10+, C++17
- **运行时**: libcurl, zlib
- **服务注册中心**: Nacos >= 2.0
- **自动下载**: muduo, nacos-sdk-cpp, nlohmann/json, protobuf 通过 CMake FetchContent 自动下载

## 使用问题

### 如何定义一个新的 RPC 服务？

```cpp
// 1. 在头文件中声明服务类
class MyService {
public:
    int add(int a, int b);
    RPC_SERVICE_BIND(MyService, add);
};

// 2. 在 .cc 文件中实现方法
int MyService::add(int a, int b) {
    return a + b;
}
```

### 为什么客户端调用时抛出 RpcException？

常见原因：
1. **服务端未启动** — 确认 RpcServer 正在运行
2. **未初始化 RpcClient** — 调用前需执行 `RpcClient::GetInstance().init(nacosAddr)`
3. **服务名不匹配** — 检查服务名格式是否正确
4. **参数类型不匹配** — 编译期通过 `static_assert` 检查
5. **Nacos 服务不可用** — 确认 Nacos 地址和配置正确

### 如何调试 RPC 调用？

启用 DEBUG 级别日志：

```cpp
minirpc::Logger::GetInstanse().setLevel(minirpc::DEBUG);
```

查看日志输出中的连接建立、消息发送和接收详情。

## 高级问题

### 连接池是如何工作的？

`ConnectionManager` 管理到多个服务端节点的连接池。每个 `EndPoint` 对应一个 `TcpClient`（基于 muduo）。首次获取连接时创建（懒加载），后续复用。

### 可以自定义超时时间吗？

当前 `Invoke()` 方法使用固定的 200ms 超时。如需调整，需修改 `RpcClient::Invoke()` 中的 `get_with_timeout()` 参数。

### 服务发现是如何工作的？

使用 **Nacos 订阅模式**：
1. 首次调用某服务时，`ServiceInstanceCache` 向 Nacos 发起 subscribe 并注册 EventListener
2. Nacos 实例变更时，SDK 后台线程回调更新缓存
3. 后续 RPC 调用直接从缓存读取实例列表（无网络 IO）

### 如何部署到生产环境？

1. 编译 release 版本：`cmake -DCMAKE_BUILD_TYPE=Release ..`
2. 确保 Nacos 服务可用，配置正确的 `config.toml`
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
