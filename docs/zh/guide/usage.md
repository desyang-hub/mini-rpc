# 使用指南

本指南介绍如何在 mini-rpc 中定义服务、启动服务端和调用 RPC 方法。

## 定义服务

使用 `RPC_SERVICE_BIND` 宏在服务类中声明要暴露的 RPC 方法：

```cpp
// UserService.h
#pragma once
#include <string>

class UserService {
public:
    std::string login(const std::string& name, const std::string& pswd);
    std::string registerUser(const std::string& name, const std::string& pswd);

    // 将 login 和 registerUser 方法暴露为 RPC 接口
    RPC_SERVICE_BIND(UserService, login, registerUser);
};
```

`RPC_SERVICE_BIND` 宏会自动生成：
- `GetInstance()` — 获取服务单例
- `Init()` — 初始化并注册所有方法到 RpcServer

支持 1~11 个方法的声明。

## 启动服务端

```cpp
// Server.cc
#include "minirpc/common/Config.h"
#include "minirpc/core/RpcServer.h"
#include "UserService.h"

int main() {
    // 加载配置文件（缺失时使用默认值）
    auto config = minirpc::loadConfig("config.toml");

    minirpc::RpcServer& server = minirpc::RpcServer::GetInstance();
    server.Start(config.port, "RpcServer", config.registry_address.c_str());

    // 保持运行...
    pthread_pause();
    return 0;
}
```

服务端启动后会自动向 Nacos 注册所有已绑定方法的服务。

## 客户端调用

使用 `RPC_SERVICE_STUB` 宏自动生成客户端代理类：

```cpp
// Client.cc
#include "UserService.h"
#include "minirpc/common/Config.h"
#include <iostream>

int main() {
    // 加载配置并初始化 RpcClient
    auto config = minirpc::loadConfig("config.toml");
    minirpc::RpcClient::GetInstance().init(config.registry_address);

    UserService::UserService_Stub stub;

    // 调用远程登录方法
    std::string result = stub.login("root", "password");
    std::cout << "Login: " << result << std::endl;

    return 0;
}
```

`RPC_SERVICE_STUB` 自动生成的 `UserService_Stub` 类具有以下方法：
- `login(name, pswd)` — 序列化参数，发送 RPC 请求，等待响应，返回结果
- `registerUser(name, pswd)` — 同上

支持 1~12 个方法的代理类生成。

## 配置文件

在每个可执行文件同级目录下放置 `config.toml`：

```toml
[server]
port = 8083
listen_host = "0.0.0.0"

[registry]
address = "127.0.0.1:8848"
group = "DefaultGroup"
cluster = "DefaultCluster"
```

配置项说明：

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `server.port` | 8080 | 服务端监听端口 |
| `server.listen_host` | 0.0.0.0 | 监听地址 |
| `registry.address` | 127.0.0.1 | Nacos 服务地址 |
| `registry.group` | DefaultGroup | Nacos 分组 |
| `registry.cluster` | DefaultCluster | Nacos 集群名 |

## 宏详解

### RPC_SERVICE_BIND

**位置**: 类定义内部（末尾）

**功能**:
1. 创建服务单例 `GetInstance()`
2. 生成自动初始化器 `_AutoInit`
3. 为每个声明的方法生成 `RpcServer::RegisterService()` 调用
4. 方法签名通过 `function_traits` 自动提取

**示例**:
```cpp
class Calculator {
public:
    int add(int a, int b);
    double divide(double a, double b);

    RPC_SERVICE_BIND(Calculator, add, divide);
};
// 注册的服务名: "Calculator.add", "Calculator.divide"
```

### RPC_SERVICE_STUB

**位置**: 客户端代码中（与服务类同头文件）

**功能**:
1. 生成 `Class##_Stub` 代理类
2. 为每个声明的方法生成客户端代理方法
3. 参数类型通过 `static_assert` 进行编译期检查
4. 异常时抛出 `RpcException`

**示例**:
```cpp
Calculator::Calculator_Stub stub;
int result = stub.add(1, 2);           // 返回 3
double result2 = stub.divide(10.0, 3); // 返回 3.333...
```

## 序列化类型

mini-rpc 支持两种序列化方式，根据参数类型自动选择：

### JSON 序列化

适用于基本类型和标准容器：

```cpp
class MyService {
public:
    std::string greet(const std::string& name);
    int add(int a, int b);
    std::vector<int> getIds();
    RPC_SERVICE_BIND(MyService, greet, add, getIds);
};
```

### Protobuf 序列化

适用于继承自 `google::protobuf::Message` 的类型：

```cpp
// 定义 protobuf 消息
message UserRequest {
    string name = 1;
    string email = 2;
    int32 age = 3;
}

message UserResponse {
    int32 id = 1;
    string message = 2;
}

class UserService {
public:
    UserResponse createUser(UserRequest req);
    RPC_SERVICE_BIND(UserService, createUser);
};
```

## 异常处理

客户端调用失败时会抛出 `RpcException`：

```cpp
try {
    std::string result = stub.login("admin", "wrong");
} catch (const minirpc::RpcException& e) {
    std::cerr << "RPC Error: " << e.what() << std::endl;
}
```

常见异常原因：
1. **服务端未启动** — 确认 RpcServer 正在运行
2. **服务名不匹配** — 检查服务名是否正确
3. **Nacos 服务不可用** — 确认 Nacos 地址和配置正确
4. **超时** — 默认 200ms 超时，网络延迟高时可能超时
