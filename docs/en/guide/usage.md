# Usage Guide

This guide explains how to define services, start a server, and call RPC methods in mini-rpc.

## Defining a Service

Use the `RPC_SERVICE_BIND` macro to declare methods to expose as RPC endpoints:

```cpp
// UserService.h
#pragma once
#include <string>

class UserService {
public:
    std::string login(const std::string& name, const std::string& pswd);
    std::string register(const std::string& name, const std::string& pswd);

    // Expose login and register methods as RPC endpoints
    RPC_SERVICE_BIND(UserService, login, register);
};
```

The `RPC_SERVICE_BIND` macro auto-generates:
- `GetInstance()` — Get the service singleton
- `Init()` — Initialize and register all methods to RpcServer

## Registering a Service

Use `RPC_SERVICE_REGISTER` in the corresponding `.cc` file:

```cpp
// UserService.cc
#include "UserService.h"
#include "minirpc/core/macro/rpc_service_register.h"

std::string UserService::login(const std::string& name, const std::string& pswd) {
    // Business logic...
    return "success";
}

std::string UserService::register(const std::string& name, const std::string& pswd) {
    // Business logic...
    return "success";
}

// Register service with RpcServer
RPC_SERVICE_REGISTER(UserService);
```

`RPC_SERVICE_REGISTER` uses a global static variable constructor to auto-call `UserService::Init()` at program startup.

## Starting the Server

```cpp
// Server.cc
#include "minirpc/net/TcpServer.h"
#include "UserService.cc"  // Ensure service is registered

int main() {
    minirpc::TcpServer server;
    server.serve(8081);  // Listen on port 8081
    return 0;
}
```

The server automatically registers all bound services with Nacos on startup.

## Client Invocation

The `RPC_SERVICE_STUB` macro auto-generates a client proxy class:

```cpp
// Client.cc
#include "UserService.h"
#include <iostream>

int main() {
    UserService::UserService_Stub stub;

    // Call remote login method
    std::string result = stub.login("root", "password");
    std::cout << "Login: " << result << std::endl;

    // Call remote register method
    std::string result2 = stub.register("newuser", "password123");
    std::cout << "Register: " << result2 << std::endl;

    return 0;
}
```

The auto-generated `UserService_Stub` class provides:
- `login(name, pswd)` — Serialize params, send RPC request, wait for response, return result
- `register(name, pswd)` — Same as above

## Macro Reference

### RPC_SERVICE_BIND

**Location**: Inside class definition (at the end)

**Functionality**:
1. Creates service singleton `GetInstance()`
2. Generates auto-initializer `_AutoInit`
3. Generates `RpcServer::Bind()` call for each declared method
4. Method signatures auto-extracted via `function_traits`

**Example**:
```cpp
class Calculator {
public:
    int add(int a, int b);
    double divide(double a, double b);

    RPC_SERVICE_BIND(Calculator, add, divide);
};
// Registered service names: "Calculator.add", "Calculator.divide"
```

### RPC_SERVICE_STUB

**Location**: Client code (same header as the service class)

**Functionality**:
1. Generates `Class##_Stub` proxy class
2. Generates client proxy method for each declared method
3. Parameter types checked at compile-time via `static_assert`
4. Throws `RpcException` on failure

**Example**:
```cpp
Calculator::Calculator_Stub stub;
int result = stub.add(1, 2);           // Returns 3
double result2 = stub.divide(10.0, 3); // Returns 3.333...
```

### RPC_SERVICE_REGISTER

**Location**: Corresponding `.cc` file

**Functionality**:
1. Creates global static initializer in namespace scope
2. Leverages C++ static initialization order to ensure `Init()` runs before `main()`
3. Triggers `RpcServer::RegisterService()` and all `Bind()` calls

## Custom Parameter Types

mini-rpc supports any C++ type that can be serialized by nlohmann/json:

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

## Exception Handling

Client call failures throw `RpcException`:

```cpp
try {
    std::string result = stub.login("admin", "wrong");
} catch (const minirpc::RpcException& e) {
    std::cerr << "RPC Error: " << e.what() << std::endl;
}
```
