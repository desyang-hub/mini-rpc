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

构建时 CMake 会通过 FetchContent 自动下载以下依赖：
- **muduo** — 高性能网络库
- **nacos-sdk-cpp** — Nacos C++ SDK
- **nlohmann/json** — JSON 序列化库
- **protobuf** — Protobuf 序列化支持
- **toml11** / **toml++** — TOML 配置文件解析

构建完成后，将在 `build/examples/` 目录下生成各示例的可执行文件。

## 运行示例

### 1. 启动 Nacos 服务

确保 Nacos 服务正在运行：

```bash
# 默认地址：127.0.0.1:8848
# 可通过配置文件 config.toml 修改
```

### 2. 启动服务端

```bash
# 在示例目录下，确保 config.toml 存在
./server_p &
```

服务端启动后会自动向 Nacos 注册服务。

### 3. 运行客户端

```bash
# 新开一个终端
./client_p
```

输出：

```
Login: success
Register: success
```

## 配置文件

每个可执行文件同级目录下放置 `config.toml`，框架会自动读取：

```toml
[server]
port = 8083
listen_host = "0.0.0.0"

[registry]
address = "127.0.0.1:8848"
group = "DefaultGroup"
cluster = "DefaultCluster"
```

配置文件缺失时不报错，使用默认值。

## 项目结构

```
mini-rpc/
├── include/minirpc/          # 公共头文件
│   ├── common/               # 通用组件（线程池、日志、缓冲区、配置）
│   ├── core/                 # RPC 核心（客户端、服务端、连接管理）
│   │   └── macro/            # 服务绑定宏
│   ├── net/                  # 网络层（基于 muduo 封装）
│   └── protocol/             # 协议层（序列化、编解码）
├── src/minirpc/              # 实现文件
├── examples/                 # 示例代码
├── tests/                    # 单元测试
├── docs/                     # 文档
├── CMakeLists.txt
└── README.md
```

## 下一步

- [架构概览](./architecture) — 了解 mini-rpc 的分层架构设计
- [使用指南](./usage) — 深入学习服务定义与 RPC 调用
- [API 参考](../api/reference) — 查看完整的 API 文档
