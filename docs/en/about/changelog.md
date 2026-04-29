# Changelog

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
