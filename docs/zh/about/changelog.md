# 更新日志

## v2.0 (2026-06-02)

### 安全审计与修复

本次发布为全面的安全审计版本，修复了 12 个漏洞，涵盖数据竞争、协议完整性、缓冲区溢出等问题。

#### 严重漏洞修复

- **TcpServer::connMap_ 数据竞争** — 为 `connMap_` 添加 `std::shared_mutex` 保护，修复 accept 回调与 removeConn 之间的并发读写竞争
- **RpcClient GetInstance() 数据竞争** — 使用 mutex-guarded 构造替代双重检查锁定，修复多线程下的双重构造问题
- **CRC32 覆盖范围扩展** — 将 CRC32 校验扩展为覆盖 `srv_name + body`，而非仅 body，防止服务名篡改攻击
- **getConnection 无界递归** — 将递归重连改为有限次数的重试循环，防止持久化故障下的栈溢出

#### 高危漏洞修复

- **LOG_FATAL 缓冲区溢出** — 修复 `snprintf` 中第二参数使用固定 1024 而非剩余空间的问题
- **Nacos URL 注入** — 对服务名进行 URL 编码后再拼接 HTTP 请求，防止特殊字符破坏或注入查询参数
- **RpcConnection::close() 数据竞争** — 将 `closed_` 改为 `std::atomic<bool>`，修复健康检查与关闭操作的并发问题
- **TcpServer 析构函数不安全** — 在析构时先退出事件循环再清理 connMap_，避免双重释放

#### 中危漏洞修复

- **getConnection for 循环逻辑错误** — 修复 pop 操作导致只检查前 N/2 个元素的问题
- **TcpServer::ClienHandler 部分发送** — 添加 send 循环确保全部数据发送完成
- **Random 使用不安全 rand()** — 替换为 mt19937 + uniform_int_distribution，支持线程安全
- **RingBuffer::read_fd 缺少 EINTR 处理** — 对 `::read()`/`::readv()` 添加 EINTR 重试

### 测试

- 新增 `test_security` 测试套件，包含 10 个回归测试
- 增强 `test_protocol`、`test_net` 和 `test_core` 测试覆盖率

### 文档

- README 新增安全考虑章节
- 更新协议文档中 CRC32 覆盖范围说明

---

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
