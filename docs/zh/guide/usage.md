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
    std::string register(const std::string& name, const std::string& pswd);

    // 将 login 和 register 方法暴露为 RPC 接口
    RPC_SERVICE_BIND(UserService, login, register);
};
```

`RPC_SERVICE_BIND` 宏会自动生成：
- `GetInstance()` — 获取服务单例
- `Init()` — 初始化并注册所有方法到 RpcServer

## 注册服务

在对应的 `.cc` 文件中使用 `RPC_SERVICE_REGISTER` 宏：

```cpp
// UserService.cc
#include "UserService.h"
#include "minirpc/core/macro/rpc_service_register.h"

std::string UserService::login(const std::string& name, const std::string& pswd) {
    // 业务逻辑...
    return "success";
}

std::string UserService::register(const std::string& name, const std::string& pswd) {
    // 业务逻辑...
    return "success";
}

// 注册服务到 RpcServer
RPC_SERVICE_REGISTER(UserService);
```

`RPC_SERVICE_REGISTER` 利用全局静态变量的构造函数在程序启动时自动调用 `UserService::Init()`。

## 启动服务端

```cpp
// Server.cc
#include "minirpc/net/TcpServer.h"
#include "UserService.cc"  // 确保服务被注册

int main() {
    minirpc::TcpServer server;
    server.serve(8081);  // 监听端口 8081
    return 0;
}
```

服务端启动后会自动向 Nacos 注册所有已绑定方法的服务。

## 客户端调用

使用 `RPC_SERVICE_STUB` 宏自动生成客户端代理类：

```cpp
// Client.cc
#include "UserService.h"
#include <iostream>

int main() {
    UserService::UserService_Stub stub;

    // 调用远程登录方法
    std::string result = stub.login("root", "password");
    std::cout << "Login: " << result << std::endl;

    // 调用远程注册方法
    std::string result2 = stub.register("newuser", "password123");
    std::cout << "Register: " << result2 << std::endl;

    return 0;
}
```

`RPC_SERVICE_STUB` 自动生成的 `UserService_Stub` 类具有以下方法：
- `login(name, pswd)` — 序列化参数，发送 RPC 请求，等待响应，返回结果
- `register(name, pswd)` — 同上

## 宏详解

### RPC_SERVICE_BIND

**位置**: 类定义内部（末尾）

**功能**:
1. 创建服务单例 `GetInstance()`
2. 生成自动初始化器 `_AutoInit`
3. 为每个声明的方法生成 `RpcServer::Bind()` 调用
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

### RPC_SERVICE_REGISTER

**位置**: 对应的 `.cc` 文件

**功能**:
1. 在命名空间作用域创建全局静态初始化器
2. 利用 C++ 静态初始化顺序保证 `Init()` 在 `main()` 之前执行
3. 触发 `RpcServer::RegisterService()` 和所有 `Bind()` 调用

## 自定义参数类型

mini-rpc 支持任意可通过 nlohmann/json 序列化的 C++ 类型：

```cpp
struct UserRequest {
    std::string name;
    std::string email;
    int age;
};

struct UserResponse {
    int id;
    std::string message;
};

class UserServer {
public:
    UserResponse createUser(const UserRequest& req);
    RPC_SERVICE_BIND(UserServer, createUser);
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
