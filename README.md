# Mini-Rpc

![Release Download](https://img.shields.io/github/downloads/desyang-hub/mini-rpc/total?style=flat-square)
[![Release Version](https://img.shields.io/github/v/release/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/releases/latest)
[![GitHub license](https://img.shields.io/github/license/desyang-hub/mini-rpc?style=flat-square)](LICENSE)
[![GitHub Star](https://img.shields.io/github/stars/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/stargazers)
[![GitHub Fork](https://img.shields.io/github/forks/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/network/members)
![GitHub Repo size](https://img.shields.io/github/repo-size/desyang-hub/mini-rpc?style=flat-square&color=3cb371)
[![Build Status](https://github.com/desyang-hub/mini-rpc/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/desyang-hub/mini-rpc/actions)
[![Build Status](https://github.com/desyang-hub/mini-rpc/actions/workflows/release.yml/badge.svg)](https://github.com/desyang-hub/mini-rpc/actions)

**[📖 中文文档](https://desyang-hub.github.io/mini-rpc/zh/)** | **[📖 English Docs](https://desyang-hub.github.io/mini-rpc/en/)** | **[📄 API 文档](https://desyang-hub.github.io/mini-rpc/zh/api/reference.html)**

Mini-Rpc is a lightweight C++ RPC framework with simple APIs, high-performance async communication, and Nacos service registry integration.

## Core Features

- **🚀 Simple API** — Macro-based service binding and stub generation
- **⚡ Async Communication** — High-performance epoll ET mode network I/O
- **🔌 Flexible Serialization** — JSON (nlohmann/json) with Protobuf reserved
- **🧵 Thread Pool** — Built-in async task execution
- **📦 Service Discovery** — Nacos integration for auto registration & discovery
- **📦 Connection Pool** — Automatic TCP connection lifecycle management

## Quick Start

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libgtest-dev libcurl4-openssl-dev zlib1g-dev
sudo apt install -y protobuf-compiler libprotobuf-dev libprotoc-dev
```

### Build

```bash
git clone https://github.com/desyang-hub/mini-rpc.git
cd mini-rpc
cmake -B build && cmake --build build -j$(nproc)
```

### Run Example

```bash
# Start Nacos (default: 127.0.0.1:8848)
# Start server
./example_server &
# Start client
./example_client
```

### Define a Service

```cpp
// Define service
class UserService {
public:
    std::string login(const std::string& name, const std::string& pswd);
    RPC_SERVICE_BIND(UserService, login);
};

// Register service
RPC_SERVICE_REGISTER(UserService);

// Client stub
UserService::UserService_Stub stub;
std::string result = stub.login("root", "password");
```

## Documentation

- [Getting Started](https://desyang-hub.github.io/mini-rpc/zh/guide/getting-started.html)
- [Architecture](https://desyang-hub.github.io/mini-rpc/zh/guide/architecture.html)
- [Usage Guide](https://desyang-hub.github.io/mini-rpc/zh/guide/usage.html)
- [Protocol](https://desyang-hub.github.io/mini-rpc/zh/guide/protocol.html)
- [API Reference](https://desyang-hub.github.io/mini-rpc/zh/api/reference.html)
- [Nacos Integration](https://desyang-hub.github.io/mini-rpc/zh/deploy/nacos.html)

## Security Considerations

### Protocol Security

- **CRC32 Integrity**: All wire-format messages include a CRC32 checksum covering both the service name and body, preventing tampering attacks that could redirect requests to unintended handlers. Tampered packets are rejected at the decoder level.
- **No Authentication/TLS**: The protocol does not provide authentication or encryption. Deploy behind a trusted network boundary or use a TLS-terminating proxy (e.g., Nginx, Envoy) for production environments.
- **Magic Number Validation**: Each message starts with a 2-byte magic number (`0x5250`). Packets with invalid magic are discarded.

### Thread Safety

- **Singleton Initialization**: `RpcClient::GetInstance()` and `Logger::GetInstance()` are thread-safe (mutex-guarded or C++17 static local guarantees).
- **Connection State**: `RpcConnection::close()` uses `std::atomic<bool>` for the `closed_` flag to prevent data races between health checks and connection teardown.
- **TcpServer Connection Map**: `connMap_` is protected by a `std::shared_mutex` to prevent concurrent modification from the event loop and thread pool workers.
- **Random Number Generation**: `Random::RandInt()` uses `std::mt19937` with a `std::mutex` for thread-safe, unbiased random number generation.

### Input Validation

- **Nacos Service Names**: Service names are URL-encoded before being embedded in Nacos API requests, preventing query parameter injection.
- **Body Size Limit**: The decoder enforces a maximum body size of 64MB to prevent memory exhaustion attacks.
- **Logging**: `LOG_FATAL` uses correct buffer size calculations to prevent buffer overflows, even with long file paths or function names.

### Known Limitations

- No built-in rate limiting or DoS protection at the protocol level.
- Connection retries are capped at 3 attempts to prevent infinite retry loops.
- `rand()` has been replaced with `std::mt19937` for cryptographic safety in service instance selection (load balancing via random choice).

## License

[MIT License](LICENSE)
