# API Reference

## RpcClient

Singleton RPC client for making remote calls.

### Get Instance

```cpp
minirpc::RpcClient& client = minirpc::RpcClient::GetInstance();
```

### Initialization

```cpp
// Set Nacos address and initialize service instance cache (call once before any RPC invocation)
client.init("127.0.0.1");
```

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `init` | `void init(const std::string& nacosAddr)` | Initialize Nacos address and service instance cache |
| `Call` | `template<class R, class ...Args> R Call(const char* serviceName, const char* name, Args&& ...args)` | Synchronous call, returns result |
| `AsyncInvoke` | `std::future<Response> AsyncInvoke(const char* name, const Bytes& bytes, uint64_t request_id)` | Async call, returns future |
| `Invoke` | `template<class R> R Invoke(const char* name, const Bytes& bytes, uint64_t request_id)` | Synchronous call with encoded request |
| `MessageHandler` | `void MessageHandler(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp t)` | Process incoming response messages |

### Call Method

```cpp
template<class R, class ...Args>
R Call(const char* serviceName, const char* methodName, Args&& ...args);
```

**Parameters**:
- `serviceName` — Service name (e.g. `UserService.login`)
- `methodName` — Method name
- `args` — Variable arguments, pass method parameters directly

**Returns**: Call result, type specified by template parameter `R`

**Timeout**: 200ms default, throws `std::runtime_error` on timeout

**Exceptions**: Throws `RpcException` when status code is not SUCCESS

---

## RpcServer

Server-side RPC handler for registering and managing service methods.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `Start` | `void Start(int port, const char* name, const char* nacosAddr)` | Start the server |
| `Stop` | `void Stop()` | Stop the server |
| `GetInstance` | `static RpcServer& GetInstance()` | Get singleton instance |
| `RegisterService` | `template<class R, class F, typename ...Args> void RegisterService(const char* className, const char* name, F&& f)` | Register a service method |
| `Invock` | `bool Invock(const std::string& srvName, const std::string& req, std::string& resp)` | Invoke a registered method |
| `addServiceInstance` | `void addServiceInstance(const char* name, const char* groupName, const char* clusterName)` | Add a service instance to register |

### Start Method

```cpp
void Start(int port = 8080,
           const char* name = "TcpServer",
           const char* nacosAddr = "127.0.0.1");
```

**Parameters**:
- `port` — Listening port
- `name` — Server name
- `nacosAddr` — Nacos registry address

**Notes**:
- Uses muduo EventLoopThread for I/O event handling
- Auto-registers services with Nacos
- Request processing delegated to ThreadPool

---

## TcpServer

TCP server based on muduo.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| Constructor | `TcpServer(int port, const char* name, size_t threadNum)` | Create server |
| `Start` | `void Start()` | Start the server |
| `Stop` | `void Stop()` | Stop the server |
| `setMessageCallback` | `void setMessageCallback(const muduo::net::MessageCallback& cb)` | Set message callback |

---

## ConnectionManager

Connection manager that maintains connection pools to multiple server endpoints.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getConnection` | `TcpClientPtr getConnection(const EndPoint& ep)` | Get connection to a specific endpoint |
| `getConnection` | `TcpClientPtr getConnection(const std::vector<EndPoint>& eps)` | Randomly select from endpoints and get connection |
| `setMessageCallback` | `void setMessageCallback(muduo::net::MessageCallback cb)` | Set message callback |
| `recovery` | `void recovery(TcpClientPtr ptr)` | Return connection to pool |

---

## ServiceInstanceCache

Nacos service instance subscription cache, updated in real-time via `EventListener` pattern.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `subscribeService` | `void subscribeService(const std::string& serviceName)` | Subscribe to a service |
| `getInstances` | `std::list<nacos::Instance> getInstances(const std::string& serviceName)` | Get instance list (from cache, non-blocking) |
| `refreshInstance` | `void refreshInstance(const std::string& serviceName)` | Manually refresh instance list |
| `shutdown` | `void shutdown()` | Forcefully destroy, avoid static destruction order issues |

---

## Config

TOML configuration file loader.

### Data Structure

```cpp
struct Config {
    int port = 8080;
    std::string listen_host = "0.0.0.0";
    std::string registry_address = "127.0.0.1";
    std::string registry_group = "DefaultGroup";
    std::string registry_cluster = "DefaultCluster";
};
```

### Load Config

```cpp
#include "minirpc/common/Config.h"

// Load config.toml from current directory
auto config = minirpc::loadConfig();

// Load config from specific path
auto config = minirpc::loadConfig("/path/to/config.toml");
```

Uses default values when config file is missing or keys are absent.

---

## Serialization

### Serialize

Automatically selects serialization based on type.

| Method | Signature | Description |
|--------|-----------|-------------|
| `Serialization` | `template<class T> static std::string Serialization(const T& obj)` | Serialize object |
| `Deserialization` | `template<class T> static T Deserialization(const std::string& data)` | Deserialize from string |
| `Deserialization` | `template<class T> static T Deserialization(const void* data, size_t len)` | Deserialize from bytes |

**Automatic Type Detection**:
- `google::protobuf::Message` derived types → Protobuf serialization
- Other types → JSON serialization

---

## Macro Reference

### RPC_SERVICE_BIND(Class, ...methods)

Declare methods to expose in the service class. Supports 1-11 methods.

### RPC_SERVICE_STUB(Class, ...methods)

Generate client proxy class `Class##_Stub`. Supports 1-12 methods.

---

## Logger

| Method | Signature | Description |
|--------|-----------|-------------|
| `setLevel` | `void setLevel(LogLevel level)` | Set log level |
| `enable_async_log_write` | `void enable_async_log_write()` | Enable async log writing |

### Log Levels

| Level | Macro | Description |
|-------|-------|-------------|
| FATAL | `LOG_FATAL(msg)` | Fatal error |
| ERROR | `LOG_ERROR(msg)` | Error |
| INFO | `LOG_INFO(msg)` | Information |
| DEBUG | `LOG_DEBUG(msg)` | Debug |

---

## ThreadPool

| Method | Signature | Description |
|--------|-----------|-------------|
| `enqueue` | `template<typename F, typename... Args> std::future<...> enqueue(F&& f, Args&&... args)` | Enqueue a task to the thread pool |

---

## Response

RPC response structure.

```cpp
struct Response {
    uint8_t state;      // Status code: SUCCESS=0, FAILED=1, TIMEOUT=2
    std::string data;   // Response data
};
```

---

## EndPoint

Network endpoint structure.

```cpp
struct EndPoint {
    std::string host;
    int port;
};
```
