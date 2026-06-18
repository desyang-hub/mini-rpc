# Nacos Integration

mini-rpc deeply integrates with [Nacos](https://nacos.io/) as a service registry for automatic service registration and discovery.

## Configuration

### Config File

Configure Nacos connection parameters via `config.toml`:

```toml
[registry]
address = "127.0.0.1"
group = "DefaultGroup"
cluster = "DefaultCluster"
```

### Code Configuration

```cpp
// Server
minirpc::RpcServer& server = minirpc::RpcServer::GetInstance();
server.Start(8083, "RpcServer", "127.0.0.1");

// Client
minirpc::RpcClient::GetInstance().init("127.0.0.1");
```

## Service Registration

When the server starts, it automatically registers all bound RPC services with Nacos:

```
RpcServer::Start(port, name, nacosAddr)
  └── Background thread (ServiceRegisterWorker)
       └── Iterate all instances to register
            └── Register as ephemeral instances with Nacos
                 └── IP: Server address
                 └── Port: Listening port
                 └── Group: Configured group
                 └── Cluster: Configured cluster name
```

Registered services are visible in the Nacos console:

```
Services > UserService
├── Cluster: DEFAULT
├── Instance: 127.0.0.1:8083 (Healthy)
└── Healthy Instances: 1
```

## Service Discovery — Subscription Mode

The client uses **Nacos subscription mode** via `ServiceInstanceCache`:

```
RpcClient::AsyncInvoke()
  └── ServiceInstanceCache::subscribeService(serviceName)
       └── First subscription: Register EventListener with Nacos
       └── Subsequent calls: Read directly from cache (no network IO)
  └── ServiceInstanceCache::getInstances(serviceName)
       └── Read instance list from cache
       └── ConnectionManager randomly selects healthy connection
       └── Establish TCP connection
```

### Subscription Mode Advantages

- **Zero blocking**: Instance changes are pushed to cache by Nacos SDK background thread
- **Low latency**: No network IO during RPC calls
- **Real-time**: Nacos pushes change notifications, cache updates immediately

### ServiceChangeListener

```cpp
class ServiceChangeListener : public nacos::EventListener {
    void receiveNamingInfo(const nacos::ServiceInfo& serviceInfo) override {
        // Nacos background thread callback, updates cache
        std::string name = std::string(info.getName());
        std::list<nacos::Instance> hosts = info.getHosts();
        instanceCache_[name] = std::move(hosts);
    }
};
```

## Multi-Service Deployment

Multiple servers can register the same service name with Nacos for load balancing:

```
Nacos
├── UserService
│   ├── 192.168.1.10:8083 (Healthy)
│   ├── 192.168.1.11:8083 (Healthy)
│   └── 192.168.1.12:8083 (Healthy)
└── OrderService
    ├── 192.168.1.10:8083 (Healthy)
    └── 192.168.1.11:8083 (Healthy)
```

Clients get all healthy instances via `ServiceInstanceCache`, and `ConnectionManager` randomly selects a connection.

## Docker Deployment for Nacos

```yaml
# docker-compose.yaml
version: '3'
services:
  nacos:
    image: nacos/nacos-server:v2.3.1
    environment:
      - MODE=standalone
    ports:
      - "8848:8848"
      - "9848:9848"
    volumes:
      - ./data:/home/nacos/data
```

```bash
docker-compose up -d
```

## Nacos-Related Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| nacos-sdk-cpp | v1.1.1 | Nacos C++ SDK |
| nacos-cli-static | - | Nacos CLI static library |
| curl | - | HTTP requests (used internally by SDK) |
| z | - | Compression library |
