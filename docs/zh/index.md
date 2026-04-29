---
layout: home

hero:
  name: Mini-RPC
  text: 轻量级 C++ RPC 框架
  tagline: 简单易用、高性能的分布式服务通信框架
  image:
    src: /logo.svg
    alt: Mini-RPC
  actions:
    - theme: brand
      text: 快速开始
      link: /zh/guide/getting-started
    - theme: alt
      text: GitHub
      link: https://github.com/desyang-hub/mini-rpc

features:
  - icon:
      src: /logo.svg
    title: 简单易用的 API
    details: 基于宏的自动绑定与存根生成，只需声明服务方法即可暴露为 RPC 接口
  - icon:
      src: /logo.svg
    title: 异步通信
    details: 基于 epoll ET 模式的高性能网络 I/O，事件驱动的非阻塞通信模型
  - icon:
      src: /logo.svg
    title: 灵活序列化
    details: 支持 JSON 和 Protobuf 序列化格式，通过 nlohmann/json 实现类型安全的数据交换
  - icon:
      src: /logo.svg
    title: 服务注册与发现
    details: 集成 Nacos 注册中心，支持服务的自动注册、发现和负载均衡
  - icon:
      src: /logo.svg
    title: 连接池管理
    details: 内置 TCP 连接池，自动管理连接生命周期，提升资源利用率
  - icon:
      src: /logo.svg
    title: 线程安全
    details: 内置线程池、异步日志和智能锁管理，保证多线程环境下的正确性
---
