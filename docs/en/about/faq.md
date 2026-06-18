# FAQ

## Basic Questions

### What is mini-rpc?

mini-rpc is a lightweight C++ RPC framework providing a simple and intuitive API for remote procedure calls. It uses muduo network library and Nacos service registry, making it suitable for distributed system development.

### What serialization formats are supported?

The framework **automatically selects** serialization based on parameter type:

- **JSON** (nlohmann/json) — For basic types, `std::string`, `std::tuple`, `std::vector`, etc.
- **Protobuf** — For types derived from `google::protobuf::Message`

### What dependencies are required?

- **Build**: CMake >= 3.20, GCC 9+ / Clang 10+, C++17
- **Runtime**: libcurl, zlib
- **Service Registry**: Nacos >= 2.0
- **Auto-downloaded**: muduo, nacos-sdk-cpp, nlohmann/json, protobuf via CMake FetchContent

## Usage Questions

### How do I define a new RPC service?

```cpp
// 1. Declare the service class in a header
class MyService {
public:
    int add(int a, int b);
    RPC_SERVICE_BIND(MyService, add);
};

// 2. Implement in the .cc file
int MyService::add(int a, int b) {
    return a + b;
}
```

### Why does the client throw RpcException?

Common causes:
1. **Server not running** — Ensure RpcServer is up and listening
2. **RpcClient not initialized** — Call `RpcClient::GetInstance().init(nacosAddr)` before invocation
3. **Service name mismatch** — Verify service name format is correct
4. **Parameter type mismatch** — Checked at compile-time via `static_assert`
5. **Nacos unavailable** — Confirm Nacos address and config are correct

### How do I debug RPC calls?

Enable DEBUG level logging:

```cpp
minirpc::Logger::GetInstanse().setLevel(minirpc::DEBUG);
```

Check log output for connection establishment, message sending and receiving details.

## Advanced Questions

### How does the connection pool work?

`ConnectionManager` maintains connection pools to multiple server endpoints. Each `EndPoint` has a corresponding `TcpClient` (based on muduo). Connections are created lazily on first access, then reused.

### Can I customize the timeout?

The current `Invoke()` method uses a fixed 200ms timeout. To adjust, modify the `get_with_timeout()` parameter in `RpcClient::Invoke()`.

### How does service discovery work?

Uses **Nacos subscription pattern**:
1. On first call to a service, `ServiceInstanceCache` subscribes to Nacos and registers an EventListener
2. When Nacos instances change, the SDK background thread callbacks update the cache
3. Subsequent RPC calls read the instance list directly from cache (no network IO)

### How do I deploy to production?

1. Build release version: `cmake -DCMAKE_BUILD_TYPE=Release ..`
2. Ensure Nacos is available, configure correct `config.toml`
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
