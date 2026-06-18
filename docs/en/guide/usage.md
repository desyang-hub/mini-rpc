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
    std::string registerUser(const std::string& name, const std::string& pswd);

    // Expose login and registerUser methods as RPC endpoints
    RPC_SERVICE_BIND(UserService, login, registerUser);
};
```

The `RPC_SERVICE_BIND` macro auto-generates:
- `GetInstance()` — Get the service singleton
- `Init()` — Initialize and register all methods to RpcServer

Supports 1-11 method declarations.

## Starting the Server

```cpp
// Server.cc
#include "minirpc/common/Config.h"
#include "minirpc/core/RpcServer.h"
#include "UserService.h"

int main() {
    // Load config file (uses defaults if missing)
    auto config = minirpc::loadConfig("config.toml");

    minirpc::RpcServer& server = minirpc::RpcServer::GetInstance();
    server.Start(config.port, "RpcServer", config.registry_address.c_str());

    // Keep running...
    pthread_pause();
    return 0;
}
```

The server automatically registers all bound services with Nacos on startup.

## Client Invocation

The `RPC_SERVICE_STUB` macro auto-generates a client proxy class:

```cpp
// Client.cc
#include "UserService.h"
#include "minirpc/common/Config.h"
#include <iostream>

int main() {
    // Load config and initialize RpcClient
    auto config = minirpc::loadConfig("config.toml");
    minirpc::RpcClient::GetInstance().init(config.registry_address);

    UserService::UserService_Stub stub;

    // Call remote login method
    std::string result = stub.login("root", "password");
    std::cout << "Login: " << result << std::endl;

    return 0;
}
```

The auto-generated `UserService_Stub` class provides:
- `login(name, pswd)` — Serialize params, send RPC request, wait for response, return result

Supports 1-12 method proxy generation.

## Configuration

Place `config.toml` alongside each executable:

```toml
[server]
port = 8083
listen_host = "0.0.0.0"

[registry]
address = "127.0.0.1:8848"
group = "DefaultGroup"
cluster = "DefaultCluster"
```

| Config | Default | Description |
|--------|---------|-------------|
| `server.port` | 8080 | Server listening port |
| `server.listen_host` | 0.0.0.0 | Listening address |
| `registry.address` | 127.0.0.1 | Nacos registry address |
| `registry.group` | DefaultGroup | Nacos group |
| `registry.cluster` | DefaultCluster | Nacos cluster name |

## Macro Reference

### RPC_SERVICE_BIND

**Location**: Inside class definition (at the end)

**Functionality**:
1. Creates service singleton `GetInstance()`
2. Generates auto-initializer `_AutoInit`
3. Generates `RpcServer::RegisterService()` call for each declared method
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

## Serialization

mini-rpc automatically selects serialization based on parameter type:

### JSON Serialization

For basic types and standard containers:

```cpp
class MyService {
public:
    std::string greet(const std::string& name);
    int add(int a, int b);
    RPC_SERVICE_BIND(MyService, greet, add);
};
```

### Protobuf Serialization

For types derived from `google::protobuf::Message`:

```cpp
message UserRequest {
    string name = 1;
    int32 age = 2;
}

class UserService {
public:
    UserResponse createUser(UserRequest req);
    RPC_SERVICE_BIND(UserService, createUser);
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
