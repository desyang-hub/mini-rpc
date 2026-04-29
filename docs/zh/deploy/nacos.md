# Nacos 集成

mini-rpc 深度集成 [Nacos](https://nacos.io/) 作为服务注册中心，实现服务的自动注册与发现。

## 环境配置

通过环境变量配置 Nacos 连接参数：

| 环境变量 | 默认值 | 说明 |
|---------|--------|------|
| `NACOS_SERVER_ADDR` | `127.0.0.1:8848` | Nacos 服务器地址 |
| `NACOS_SERVER_HOST` | `127.0.0.1` | Nacos 服务器主机 |
| `NACOS_SERVER_PORT` | `8848` | Nacos 服务器端口 |

```bash
# 示例：配置 Nacos 地址
export NACOS_SERVER_ADDR=192.168.1.100:8848
export NACOS_SERVER_HOST=192.168.1.100
export NACOS_SERVER_PORT=8848
```

## 服务注册

服务端启动时，会自动向 Nacos 注册所有已绑定的 RPC 服务：

```
TcpServer::serve(port)
  └── 后台线程
       └── 遍历所有已注册的 service name
            └── 向 Nacos 注册为临时实例
                 └── IP: 服务器地址
                 └── Port: 监听端口
```

注册的服务在 Nacos 控制台可见：

```
服务列表 > UserService
├── 集群: DEFAULT
├── 实例: 127.0.0.1:8081 (健康)
└── 健康实例数: 1
```

## 服务发现

客户端调用 RPC 时，自动从 Nacos 获取服务实例地址：

```
RpcConnectionPool::connect()
  └── getServiceAddress(serviceName)
       └── 调用 Nacos HTTP API
            └── GET /nacos/v1/ns/instance/list?serviceName=X
            └── 解析第一个健康实例的 ip:port
            └── 建立 TCP 连接
```

Nacos SDK 通过 libcurl 发起 HTTP 请求，返回 JSON 格式的服务列表，客户端取第一个健康实例建立连接。

## 使用 Nacos SDK 直接操作

如需直接操作 Nacos（手动注册/注销），可使用 nacos-sdk-cpp：

```bash
# FetchContent 自动下载（CMakeLists.txt 中已配置）
FetchContent_Declare(
    nacos_cpp
    GIT_REPOSITORY https://github.com/nacos-group/nacos-sdk-cpp.git
    GIT_TAG        v1.1.1
)
```

```cpp
#include "Nacos.h"

// 创建工厂和命名服务
Properties configProps;
configProps[PropertyKeyConst::SERVER_ADDR] = "127.0.0.1";
INacosServiceFactory* factory = NacosFactoryFactory::getNacosFactory(configProps);
NamingService* namingSvc = factory->CreateNamingService();

// 注册实例
Instance instance;
instance.ip = "127.0.0.1";
instance.port = 8081;
instance.ephemeral = true;
namingSvc->registerInstance("MyService", instance);

// 注销实例
namingSvc->deregisterInstance("MyService", instance);
```

## 多服务部署

多个服务端可以同时注册相同的服务名到 Nacos，实现负载均衡：

```
Nacos
├── UserService
│   ├── 192.168.1.10:8081 (健康)
│   ├── 192.168.1.11:8081 (健康)
│   └── 192.168.1.12:8081 (健康)
└── OrderService
    ├── 192.168.1.10:8082 (健康)
    └── 192.168.1.11:8082 (健康)
```

客户端通过 `getServiceAddress()` 获取第一个健康实例，可根据实际需求扩展负载均衡策略。

## Docker 部署 Nacos

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

## 与 Nacos 相关的依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| nacos-sdk-cpp | v1.1.1 | Nacos C++ SDK |
| nacos-cli-static | - | Nacos CLI 静态库 |
| curl | - | HTTP 请求（服务发现） |
| z | - | 压缩库 |
