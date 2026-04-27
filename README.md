# Mini-Rpc 项目说明

![Release Download](https://img.shields.io/github/downloads/desyang-hub/mini-rpc/total?style=flat-square)
[![Release Version](https://img.shields.io/github/v/release/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/releases/latest)
[![GitHub license](https://img.shields.io/github/license/desyang-hub/mini-rpc?style=flat-square)](LICENSE)
[![GitHub Star](https://img.shields.io/github/stars/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/stargazers)
[![GitHub Fork](https://img.shields.io/github/forks/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/network/members)
![GitHub Repo size](https://img.shields.io/github/repo-size/desyang-hub/mini-rpc?style=flat-square&color=3cb371)
[![GitHub Repo Languages](https://img.shields.io/github/languages/top/desyang-hub/mini-rpc?style=flat-square)](https://github.com/desyang-hub/mini-rpc/search?l=c%23)
[![Build Status](https://github.com/desyang-hub/mini-rpc/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/desyang-hub/mini-rpc/actions)
[![Build Status](https://github.com/desyang-hub/mini-rpc/actions/workflows/release.yml/badge.svg)](https://github.com/desyang-hub/mini-rpc/actions)
[![GitHub Star](https://img.shields.io/github/stars/desyang-hub/mini-rpc.svg?logo=github)](https://github.com/desyang-hub/mini-rpc)

## 简介
Mini-Rpc 是一个轻量级的 C++ RPC 框架，提供了简单易用的接口来实现远程过程调用。

## 核心特性
- 🚀 简单易用的 API 设计
- ⚡ 高性能的异步通信
- 🔌 支持多种序列化方式（JSON、Protobuf）
- 🧵 内置线程池支持
- 📦 单例模式的客户端设计

## 快速开始

### 依赖
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ libgtest-dev libcurl4-openssl-dev
```

### 安装
```bash
git clone https://github.com/desyang-hub/mini-rpc.git
cd mini-rpc
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 示例代码

**定义服务：**
```cpp
class UserService {
public:
    std::string login(const std::string& name, const std::string& pswd);
    std::string register(const std::string& name, const std::string& pswd);
    
    RPC_SERVICE_BIND(UserService, login, register);
    RPC_SERVICE_STUB(UserService, login, register);
};
```

**服务端：**
```cpp
minirpc::TcpServer tcpServer;
tcpServer.serve(8080);
```

**客户端：**
```cpp
UserService::UserService_Stub stub;
std::string result = stub.login("username", "password");
```


## 许可证
[MIT License](LICENSE)