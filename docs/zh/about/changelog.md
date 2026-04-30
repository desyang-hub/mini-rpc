# 更新日志

## v1.5.1 (2026-04-29)

### 重大重构

- **解耦 Rpc 与网络连接** — 将 TcpClient 中混入的 Nacos 代码移除，网络层与 RPC 层完全解耦
- **连接池管理优化** — RpcClient 中的连接和事件循环交由 TcpConnectionPool 管理，支持多线程安全
- **修复重复发送问题** — RpcClient::call 不再发送两次请求，修复了资源浪费和潜在的连接泄漏
- **修复线程管理** — RpcConnectionPool 使用 joinable 线程替代 detached 线程，添加析构函数确保资源释放

### 新增功能

- **可配置 Nacos 地址** — 支持通过环境变量 `NACOS_SERVER_ADDR`、`NACOS_SERVER_HOST`、`NACOS_SERVER_PORT` 配置 Nacos 地址
- **ET 模式消息处理** — 添加 `IConnection::processConnection` 静态辅助方法，简化 ET 模式下的批量消息处理逻辑
- **消息处理回调** — 连接池支持配置 `MessageHandler` 回调，统一处理消息分发

### Bug 修复

- 修复 `hanelers_mutex_` → `handlers_mutex_` 拼写错误
- 修复 `clsNmae` → `className` 拼写错误
- 修复 `namespace rpc` → `namespace minirpc` 命名空间错误
- 修复 `exit(-1)` → `return -1`，避免在库函数中异常终止进程
- 修复 `MessageHandler` 类型作用域问题（移到命名空间级别）
- 修复 circular dependency（将 nacos_config.cc 移至 common 库）

### 代码质量

- 删除 `TcpClient.h/.cc` 中的混入代码
- 删除无用的 `nacos/` 和 `dependense/` 目录
- 统一代码风格和命名规范
- 新增架构文档和协议文档

---

## v1.4.0

### 新增功能

- 添加 Nacos 服务注册与发现
- 添加连接池机制
- 添加异步日志系统

---

## v1.3.0

### 新增功能

- 添加 epoll ET 模式网络层
- 添加 Protobuf 序列化预留接口
- 添加线程池支持

---

## v1.0.0

初始版本，包含基础 RPC 通信框架。
