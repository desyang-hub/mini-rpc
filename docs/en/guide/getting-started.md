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

After building, executables will be in `bin/` and static libraries in `lib/`.

## Run Examples

### 1. Start Nacos Service

Ensure Nacos is running:

```bash
# Default: 127.0.0.1:8848
# Override via environment variables:
export NACOS_SERVER_ADDR=127.0.0.1:8848
export NACOS_SERVER_HOST=127.0.0.1
export NACOS_SERVER_PORT=8848
```

### 2. Start the Server

```bash
# In mini-rpc/build-test/bin
./example_server &
```

The server automatically registers services with Nacos on startup.

### 3. Run the Client

```bash
# In a new terminal
./example_client
```

Output:

```
Login: success
Register: success
```

### 4. Run minirpc_main

```bash
./minirpc_main
```

Output:

```
sum: 3
sub: -1
```

## Project Structure

```
mini-rpc/
├── include/minirpc/          # Public headers
│   ├── common/               # Utilities (ThreadPool, Logger, Buffer)
│   ├── core/                 # RPC core (Client, Server, Connection Pool)
│   │   └── macro/            # Service binding macros
│   ├── net/                  # Network layer (TCP Server, EventLoop, Epoll)
│   └── protocol/             # Protocol layer (Serialization, Encoding)
├── src/minirpc/              # Implementation files
├── example/                  # Example code
│   ├── Server.cc
│   ├── Client.cc
│   ├── UserService.h
│   └── UserService.cc
├── tests/                    # Unit tests
├── CMakeLists.txt
└── README.md
```

## Next Steps

- [Architecture](./architecture) — Understand mini-rpc's layered architecture
- [Usage](./usage) — Deep dive into service definition and RPC calls
- [API Reference](../api/reference) — Complete API documentation
