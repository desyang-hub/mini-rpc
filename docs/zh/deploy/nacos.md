# Nacos 集成

mini-rpc 深度集成 [Nacos](https://nacos.io/) 作为服务注册中心，实现服务的自动注册与发现。

## 配置方式

### 配置文件

通过 `config.toml` 配置 Nacos 连接参数：

```toml
[registry]
address = "127.0.0.1"
group = "DefaultGroup"
cluster = "DefaultCluster"
```

### 代码配置

```cpp
// 服务端
minirpc::RpcServer& server = minirpc::RpcServer::GetInstance();
server.Start(8083, "RpcServer", "127.0.0.1");

// 客户端
minirpc::RpcClient::GetInstance().init("127.0.0.1");
```

## 服务注册

服务端启动时，会自动向 Nacos 注册所有已绑定的 RPC 服务：

```
RpcServer::Start(port, name, nacosAddr)
  └── 后台线程 (ServiceRegisterWorker)
       └── 遍历所有待注册的实例
            └── 向 Nacos 注册为临时实例
                 └── IP: 服务器地址
                 └── Port: 监听端口
                 └── Group: 配置的分组
                 └── Cluster: 配置的集群名
```

注册的服务在 Nacos 控制台可见：

```
服务列表 > UserService
├── 集群: DEFAULT
├── 实例: 127.0.0.1:8083 (健康)
└── 健康实例数: 1
```

## 服务发现 — 订阅模式

客户端使用 **Nacos 订阅模式**，通过 `ServiceInstanceCache` 获取服务实例列表：

```
RpcClient::AsyncInvoke()
  └── ServiceInstanceCache::subscribeService(serviceName)
       └── 首次订阅：向 Nacos 注册 EventListener
       └── 后续调用：直接从缓存读取实例列表（无网络 IO）
  └── ServiceInstanceCache::getInstances(serviceName)
       └── 从缓存读取实例列表
       └── ConnectionManager 随机选择健康连接
       └── 建立 TCP 连接
```

### 订阅模式优势

- **零阻塞**: 实例变更由 Nacos SDK 后台线程回调更新缓存
- **低延迟**: RPC 调用时无需等待网络 IO
- **实时性**: Nacos 推送变更通知，缓存即时更新

### ServiceChangeListener

```cpp
class ServiceChangeListener : public nacos::EventListener {
    void receiveNamingInfo(const nacos::ServiceInfo& serviceInfo) override {
        // Nacos 后台线程回调，更新缓存
        std::string name = std::string(info.getName());
        std::list<nacos::Instance> hosts = info.getHosts();
        instanceCache_[name] = std::move(hosts);
    }
};
```

## 多服务部署

多个服务端可以同时注册相同的服务名到 Nacos，实现负载均衡：

```
Nacos
├── UserService
│   ├── 192.168.1.10:8083 (健康)
│   ├── 192.168.1.11:8083 (健康)
│   └── 192.168.1.12:8083 (健康)
└── OrderService
    ├── 192.168.1.10:8083 (健康)
    └── 192.168.1.11:8083 (健康)
```

客户端通过 `ServiceInstanceCache` 获取所有健康实例，`ConnectionManager` 随机选择连接。

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
| curl | - | HTTP 请求（SDK 内部使用）|
| z | - | 压缩库 |
