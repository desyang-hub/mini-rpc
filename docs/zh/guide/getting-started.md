# 快速开始

本指南将帮助你快速搭建 mini-rpc 环境，并运行第一个 RPC 示例。

## 系统要求

- **操作系统**: Linux (推荐 Ubuntu 20.04+)
- **CMake**: >= 3.20
- **C++ 编译器**: GCC 9+ 或 Clang 10+ (支持 C++17)
- **Nacos**: >= 2.0 (服务注册中心)

## 安装依赖

```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libgtest-dev libcurl4-openssl-dev zlib1g-dev
```

## 克隆并构建

```bash
git clone https://github.com/desyang-hub/mini-rpc.git
cd mini-rpc
mkdir build && cd build
cmake ..
make -j$(nproc)
```

构建完成后，将在 `bin/` 目录下生成可执行文件，`lib/` 目录下生成静态库文件。

## 运行示例

### 1. 启动 Nacos 服务

确保 Nacos 服务正在运行：

```bash
# 默认地址：127.0.0.1:8848
# 可通过环境变量修改：
export NACOS_SERVER_ADDR=127.0.0.1:8848
export NACOS_SERVER_HOST=127.0.0.1
export NACOS_SERVER_PORT=8848
```

### 2. 启动服务端

```bash
# 在 mini-rpc/build-test/bin 目录下
./example_server &
```

服务端启动后会自动向 Nacos 注册服务。

### 3. 运行客户端

```bash
# 新开一个终端
./example_client
```

输出：

```
Login: success
Register: success
```

### 4. 使用 minirpc_main

```bash
./minirpc_main
```

输出：

```
sum: 3
sub: -1
```

## 项目结构

```
mini-rpc/
├── include/minirpc/          # 公共头文件
│   ├── common/               # 通用组件（线程池、日志、缓冲区）
│   ├── core/                 # RPC 核心（客户端、服务端、连接池）
│   │   └── macro/            # 服务绑定宏
│   ├── net/                  # 网络层（TCP 服务器、事件循环、Epoll）
│   └── protocol/             # 协议层（序列化、编解码）
├── src/minirpc/              # 实现文件
├── example/                  # 示例代码
│   ├── Server.cc
│   ├── Client.cc
│   ├── UserService.h
│   └── UserService.cc
├── tests/                    # 单元测试
├── CMakeLists.txt
└── README.md
```

## 下一步

- [架构概览](./architecture) — 了解 mini-rpc 的分层架构设计
- [使用指南](./usage) — 深入学习服务定义与 RPC 调用
- [API 参考](../api/reference) — 查看完整的 API 文档
