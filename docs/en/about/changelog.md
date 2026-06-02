# Changelog

## v2.0 (2026-06-02)

### Security Audit & Fixes

This release is a comprehensive security audit, fixing 12 vulnerabilities across data races, protocol integrity, buffer overflows, and code quality issues.

#### Critical Fixes

- **TcpServer::connMap_ data race** — Added `std::shared_mutex` protection for `connMap_`, fixing concurrent read/write between accept callback and `removeConn`
- **RpcClient GetInstance() data race** — Replaced double-checked locking with mutex-guarded construction, preventing double instantiation under multi-threaded access
- **CRC32 coverage expansion** — Extended CRC32 checksum to cover `srv_name + body` instead of just body, preventing service name tampering attacks
- **getConnection unbounded recursion** — Replaced recursive reconnect with bounded retry loop, preventing stack overflow during persistent failures

#### High Fixes

- **LOG_FATAL buffer overflow** — Fixed `snprintf` second call using hardcoded 1024 instead of remaining buffer space
- **Nacos URL injection** — Added URL encoding for service names before HTTP concatenation, preventing special character injection
- **RpcConnection::close() data race** — Changed `closed_` to `std::atomic<bool>`, fixing race between health check and close operations
- **TcpServer destructor unsafe shutdown** — Destructor now quits event loop before cleaning up connMap_, avoiding double-delete

#### Medium Fixes

- **getConnection for-loop logic bug** — Fixed pop-in-loop causing only first N/2 elements to be checked
- **TcpServer::ClienHandler partial sends** — Added send loop to ensure all bytes are transmitted
- **Insecure rand() in Random** — Replaced with mt19937 + uniform_int_distribution, thread-safe
- **RingBuffer::read_fd missing EINTR handling** — Added EINTR retry for `::read()`/`::readv()`

### Testing

- New `test_security` test suite with 10 regression tests
- Enhanced `test_protocol`, `test_net`, and `test_core` test coverage

### Documentation

- Added Security Considerations section to README
- Updated protocol docs with corrected CRC32 coverage description

---

## v1.5.1 (2025-04-29)

### Major Refactoring

- **Decoupled Rpc from Network** — Removed Nacos code mixed into TcpClient, fully decoupled network layer from RPC layer
- **Connection Pool Optimization** — Connection and event loop management moved to TcpConnectionPool, supporting multi-thread safety
- **Fixed Double-Send Bug** — RpcClient::call no longer sends requests twice, fixing resource waste and potential connection leaks
- **Fixed Thread Management** — RpcConnectionPool uses joinable threads instead of detached threads, destructor ensures resource release

### New Features

- **Configurable Nacos Address** — Supports configuring Nacos via env vars: `NACOS_SERVER_ADDR`, `NACOS_SERVER_HOST`, `NACOS_SERVER_PORT`
- **ET Mode Message Processing** — Added `IConnection::processConnection` static helper for simplified batch message processing in ET mode
- **Message Handler Callback** — Connection pools support configurable `MessageHandler` callback for unified message dispatch

### Bug Fixes

- Fixed `hanelers_mutex_` → `handlers_mutex_` typo
- Fixed `clsNmae` → `className` typo
- Fixed `namespace rpc` → `namespace minirpc` namespace error
- Fixed `exit(-1)` → `return -1`, avoiding abnormal process termination in library functions
- Fixed `MessageHandler` type scope (moved to namespace level)
- Fixed circular dependency (moved nacos_config.cc to common library)

### Code Quality

- Removed mixed-in code from `TcpClient.h/.cc`
- Removed unused `nacos/` and `dependense/` directories
- Unified code style and naming conventions
- Added architecture and protocol documentation

---

## v1.4.0

### New Features

- Added Nacos service registration and discovery
- Added connection pool mechanism
- Added async logging system

---

## v1.3.0

### New Features

- Added epoll ET mode network layer
- Added Protobuf serialization reserved interface
- Added thread pool support

---

## v1.0.0

Initial release with basic RPC communication framework.
