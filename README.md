# Mini-Rpc

![Release Download](https://img.shields.io/github/downloads/desyang-hub/mini-rpc/total?style=flat-square)
[![Release Version](https://img.shields.io/github/v/release/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/releases/latest)
[![GitHub license](https://img.shields.io/github/license/desyang-hub/mini-rpc?style=flat-square)](LICENSE)
[![GitHub Star](https://img.shields.io/github/stars/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/stargazers)
[![GitHub Fork](https://img.shields.io/github/forks/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/network/members)
![GitHub Repo size](https://img.shields.io/github/repo-size/desyang-hub/mini-rpc?style=flat-square&color=3cb371)
[![Build Status](https://github.com/desyang-hub/mini-rpc/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/desyang-hub/mini-rpc/actions)
[![Build Status](https://github.com/desyang-hub/mini-rpc/actions/workflows/release.yml/badge.svg)](https://github.com/desyang-hub/mini-rpc/actions)

**[📖 中文文档](https://desyang-hub.github.io/mini-rpc/zh/)** | **[📖 English Docs](https://desyang-hub.github.io/mini-rpc/en/)**

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
```

### Build

```bash
git clone https://github.com/desyang-hub/mini-rpc.git
cd mini-rpc
mkdir build && cd build
cmake ..
make -j$(nproc)
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

- [Getting Started](https://desyang-hub.github.io/mini-rpc/zh/guide/getting-started)
- [Architecture](https://desyang-hub.github.io/mini-rpc/zh/guide/architecture)
- [Usage Guide](https://desyang-hub.github.io/mini-rpc/zh/guide/usage)
- [Protocol](https://desyang-hub.github.io/mini-rpc/zh/guide/protocol)
- [API Reference](https://desyang-hub.github.io/mini-rpc/zh/api/reference)
- [Nacos Integration](https://desyang-hub.github.io/mini-rpc/zh/deploy/nacos)

## License

[MIT License](LICENSE)
