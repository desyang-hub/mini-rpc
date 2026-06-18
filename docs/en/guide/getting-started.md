# Getting Started

This guide will help you set up mini-rpc and run your first RPC example.

## System Requirements

- **OS**: Linux (Ubuntu 20.04+ recommended)
- **CMake**: >= 3.20
- **C++ Compiler**: GCC 9+ or Clang 10+ (C++17 support)
- **Nacos**: >= 2.0 (Service registry)

## Install Dependencies

```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libgtest-dev libcurl4-openssl-dev zlib1g-dev
```

## Clone & Build

```bash
git clone https://github.com/desyang-hub/mini-rpc.git
cd mini-rpc
mkdir build && cd build
cmake ..
make -j$(nproc)
```

CMake automatically downloads the following dependencies via FetchContent:
- **muduo** — High-performance network library
- **nacos-sdk-cpp** — Nacos C++ SDK
- **nlohmann/json** — JSON serialization
- **protobuf** — Protobuf serialization support
- **toml++** — TOML config file parsing

After building, executables will be in `build/examples/`.

## Configuration

Place a `config.toml` next to each executable:

```toml
[server]
port = 8083
listen_host = "0.0.0.0"

[registry]
address = "127.0.0.1:8848"
group = "DefaultGroup"
cluster = "DefaultCluster"
```

Default values are used when the config file is missing.

## Run Examples

### 1. Start Nacos Service

Ensure Nacos is running at `127.0.0.1:8848` (default).

### 2. Start the Server

```bash
cd build/examples/simple_plus
./server_p &
```

The server automatically registers services with Nacos on startup.

### 3. Run the Client

```bash
# In a new terminal
./client_p
```

Output:

```
Login: success
Register: success
```

## Project Structure

```
mini-rpc/
├── include/minirpc/          # Public headers
│   ├── common/               # Utilities (ThreadPool, Logger, Config)
│   ├── core/                 # RPC core (Client, Server, Connection Manager)
│   │   └── macro/            # Service binding macros
│   ├── net/                  # Network layer (muduo-based)
│   └── protocol/             # Protocol layer (Serialization, Encoding)
├── src/minirpc/              # Implementation files
├── examples/                 # Example code
├── tests/                    # Unit tests
├── docs/                     # Documentation
├── CMakeLists.txt
└── README.md
```

## Next Steps

- [Architecture](./architecture) — Understand mini-rpc's layered architecture
- [Usage](./usage) — Deep dive into service definition and RPC calls
- [API Reference](../api/reference) — Complete API documentation
