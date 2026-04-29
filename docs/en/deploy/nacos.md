# Nacos Integration

mini-rpc deeply integrates with [Nacos](https://nacos.io/) as a service registry for automatic service registration and discovery.

## Environment Configuration

Configure Nacos connection parameters via environment variables:

| Environment Variable | Default | Description |
|---------------------|---------|-------------|
| `NACOS_SERVER_ADDR` | `127.0.0.1:8848` | Nacos server address |
| `NACOS_SERVER_HOST` | `127.0.0.1` | Nacos server host |
| `NACOS_SERVER_PORT` | `8848` | Nacos server port |

```bash
# Example: Configure Nacos address
export NACOS_SERVER_ADDR=192.168.1.100:8848
export NACOS_SERVER_HOST=192.168.1.100
export NACOS_SERVER_PORT=8848
```

## Service Registration

When the server starts, it automatically registers all bound RPC services with Nacos:

```
TcpServer::serve(port)
  └── Background thread
       └── Iterate all registered service names
            └── Register as ephemeral instances with Nacos
                 └── IP: Server address
                 └── Port: Listening port
```

Registered services are visible in the Nacos console:

```
Services > UserService
├── Cluster: DEFAULT
├── Instance: 127.0.0.1:8081 (Healthy)
└── Healthy Instances: 1
```

## Service Discovery

When the client makes an RPC call, it automatically retrieves the service instance address from Nacos:

```
RpcConnectionPool::connect()
  └── getServiceAddress(serviceName)
       └── Call Nacos HTTP API
            └── GET /nacos/v1/ns/instance/list?serviceName=X
            └── Extract first healthy instance's ip:port
            └── Establish TCP connection
```

The Nacos SDK uses libcurl to make HTTP requests, returns a JSON-formatted service list, and the client picks the first healthy instance to establish a connection.

## Direct Nacos SDK Operations

For direct Nacos operations (manual registration/deregistration), use nacos-sdk-cpp:

```bash
# Auto-downloaded via FetchContent (configured in CMakeLists.txt)
FetchContent_Declare(
    nacos_cpp
    GIT_REPOSITORY https://github.com/nacos-group/nacos-sdk-cpp.git
    GIT_TAG        v1.1.1
)
```

```cpp
#include "Nacos.h"

// Create factory and naming service
Properties configProps;
configProps[PropertyKeyConst::SERVER_ADDR] = "127.0.0.1";
INacosServiceFactory* factory = NacosFactoryFactory::getNacosFactory(configProps);
NamingService* namingSvc = factory->CreateNamingService();

// Register instance
Instance instance;
instance.ip = "127.0.0.1";
instance.port = 8081;
instance.ephemeral = true;
namingSvc->registerInstance("MyService", instance);

// Deregister instance
namingSvc->deregisterInstance("MyService", instance);
```

## Multi-Service Deployment

Multiple servers can register the same service name with Nacos for load balancing:

```
Nacos
├── UserService
│   ├── 192.168.1.10:8081 (Healthy)
│   ├── 192.168.1.11:8081 (Healthy)
│   └── 192.168.1.12:8081 (Healthy)
└── OrderService
    ├── 192.168.1.10:8082 (Healthy)
    └── 192.168.1.11:8082 (Healthy)
```

Clients retrieve the first healthy instance via `getServiceAddress()`. Load balancing strategies can be extended as needed.

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
| curl | - | HTTP requests (service discovery) |
| z | - | Compression library |
