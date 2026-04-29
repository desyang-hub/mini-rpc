# API Reference

## RpcClient

Singleton RPC client for making remote calls.

### Get Instance

```cpp
minirpc::RpcClient& client = minirpc::RpcClient::GetInstance();
```

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `call` | `template<T> bool call(const std::string& name, const args_tuple& args, T& ret)` | Synchronous call, returns status code |
| `call` | `template<> bool call(const std::string& name, const args_tuple& args)` | Synchronous call (void return), returns status code |
| `async_call` | `template<typename F, typename... Args> std::future<...> async_call(F&& f, Args&&... args)` | Async call, wrapped in std::async |
| `messageHandler` | `void messageHandler(IConnection* conn)` | Process incoming response messages |

### call Method Signatures

```cpp
// With return value
template<typename Ret, typename... Args>
bool call(const std::string& serviceName, std::tuple<Args...> args, Ret& ret);

// void return type
template<typename... Args>
bool call(const std::string& serviceName, std::tuple<Args...> args);
```

**Parameters**:
- `serviceName` — Service name in format `ClassName.methodName` (e.g. `UserService.login`)
- `args` — Argument tuple, must match the service method signature
- `ret` — Output parameter for the return value

**Returns**: `true` on success, `false` on failure (timeout/connection closed)

**Exceptions**: Throws `RpcException` when status code is not SUCCESS

---

## RpcServer

Server-side RPC handler for registering and managing service methods.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `Bind` | `void Bind(const std::string& name, RpcHandler handler)` | Bind a method to a service name |
| `Call` | `bool Call(const std::string& name, const std::string& body, std::string& res)` | Invoke a registered method |
| `RegisterService` | `void RegisterService(const std::string& serviceName)` | Register a service name with Nacos |

### Bind Method

```cpp
void Bind(const std::string& name, RpcHandler handler);
```

**Parameters**:
- `name` — Service name (e.g. `UserService.login`)
- `handler` — Handler function, type `std::function<std::string(const std::tuple<Args...>&)>`

### Call Method

```cpp
bool Call(const std::string& name, const std::string& body, std::string& res);
```

**Parameters**:
- `name` — Service name
- `body` — Request body (serialized JSON parameters)
- `res` — Output parameter for response body

**Returns**: `true` on success, `false` if method not found

---

## TcpServer

TCP server managing listening socket and client connections.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `serve` | `void serve(int port)` | Start server on the given port |

### serve Method

```cpp
void serve(int port);
```

**Parameters**:
- `port` — Listening port

**Notes**:
- Uses epoll ET mode for I/O event handling
- Auto-registers services with Nacos
- Main thread runs the event loop, request processing delegated to ThreadPool

---

## IConnection

Connection interface defining abstract connection operations.

### Core Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `send` | `virtual bool send(const std::string& data) = 0` | Send data |
| `readMsg` | `virtual int readMsg() = 0` | Read a message, returns bytes read |
| `decode` | `virtual bool decode(ProtocolHeader& header, std::string& body) = 0` | Decode a message |
| `isHealthy` | `virtual bool isHealthy() const = 0` | Check connection health |
| `fd` | `virtual int fd() const = 0` | Get file descriptor |
| `close` | `virtual void close() = 0` | Close the connection |
| `targetAddress` | `virtual std::string targetAddress() const = 0` | Get target address |
| `processConnection` | `static bool processConnection(IConnection* conn, ...)` | Batch message processing in ET mode |

---

## Serialization

### Serialize

| Method | Signature | Description |
|--------|-----------|-------------|
| `Serialization` | `static std::string Serialization(const std::tuple<Args...>& args)` | Serialize args to JSON string |
| `Deserialization` | `static T Deserialization(const std::string& body)` | Deserialize JSON string to type T |
| `SetSerializeType` | `static void SetSerializeType(SerializeType type)` | Set serialization type |

---

## Macro Reference

### RPC_SERVICE_BIND(Class, ...methods)

Declare methods to expose in the service class.

### RPC_SERVICE_STUB(Class, ...methods)

Generate client proxy class `Class##_Stub`.

### RPC_SERVICE_REGISTER(Class)

Register the service in the `.cc` file.

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
