# FAQ

## Basic Questions

### What is mini-rpc?

mini-rpc is a lightweight C++ RPC framework providing a simple and intuitive API for remote procedure calls. It uses epoll ET mode for high-performance network I/O and supports service registration/discovery (Nacos), making it suitable for distributed system development.

### What serialization formats are supported?

Currently **JSON** (via nlohmann/json). Protobuf serialization interface is defined but not implemented, leaving room for extension.

### What dependencies are required?

- **Build**: CMake >= 3.20, GCC 9+ / Clang 10+, C++17
- **Runtime**: libcurl, zlib
- **Service Registry**: Nacos >= 2.0

## Usage Questions

### How do I define a new RPC service?

```cpp
// 1. Declare the service class in a header
class MyService {
public:
    int add(int a, int b);
    RPC_SERVICE_BIND(MyService, add);
};

// 2. Register in the .cc file
RPC_SERVICE_REGISTER(MyService);
```

### Why does the client throw RpcException?

Common causes:
1. **Server not running** — Ensure TcpServer is up and listening
2. **Service name mismatch** — Verify service name format `ClassName.methodName`
3. **Parameter type mismatch** — Checked at compile-time via `static_assert`, runtime via CRC32 validation
4. **Nacos unavailable** — Confirm Nacos address and environment variables are correct

### How do I debug RPC calls?

Enable DEBUG level logging:

```cpp
minirpc::Logger::GetInstanse().setLevel(minirpc::DEBUG);
```

Check log output for connection establishment, message sending and receiving details.

## Advanced Questions

### How does the connection pool work?

Each service name corresponds to a `RpcConnectionPool`. Connections are created lazily on first borrow. Each pool has its own epoll event loop thread, automatically managing connection lifecycle (health checks, reconnect on disconnect).

### Can I customize the timeout?

The current `call()` method uses a fixed 5-second timeout. To adjust, modify the `wait_for()` parameter in `RpcClient::call()`.

### Is Protobuf supported?

The interface is defined but not yet implemented. JSON serialization is used by default. Protobuf support requires implementing the serialization/deserialization logic yourself.

### How do I deploy to production?

1. Build release version: `cmake -DCMAKE_BUILD_TYPE=Release ..`
2. Ensure Nacos is available and configure correct environment variables
3. Manage service processes with systemd or Docker
4. Monitor Nacos console for service health status

## Contribution

### How can I contribute?

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/xxx`)
3. Commit changes (`git commit -am 'Add xxx'`)
4. Push to branch (`git push origin feature/xxx`)
5. Submit a Pull Request

### What license does the project use?

[MIT License](https://github.com/desyang-hub/mini-rpc/blob/main/LICENSE)
