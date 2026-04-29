---
layout: home

hero:
  name: Mini-RPC
  text: Lightweight C++ RPC Framework
  tagline: Simple, high-performance distributed service communication framework
  image:
    src: /logo.svg
    alt: Mini-RPC
  actions:
    - theme: brand
      text: Getting Started
      link: /en/guide/getting-started
    - theme: alt
      text: GitHub
      link: https://github.com/desyang-hub/mini-rpc

features:
  - icon:
      src: /logo.svg
    title: Simple & Intuitive API
    details: Macro-based auto-binding and stub generation — declare your service methods and expose them as RPC endpoints
  - icon:
      src: /logo.svg
    title: Asynchronous Communication
    details: High-performance network I/O powered by epoll ET mode, event-driven non-blocking communication model
  - icon:
      src: /logo.svg
    title: Flexible Serialization
    details: Support for JSON and Protobuf serialization, with nlohmann/json for type-safe data exchange
  - icon:
      src: /logo.svg
    title: Service Registration & Discovery
    details: Integrated with Nacos registry for automatic service registration, discovery, and load balancing
  - icon:
      src: /logo.svg
    title: Connection Pool Management
    details: Built-in TCP connection pool with automatic lifecycle management for optimal resource utilization
  - icon:
      src: /logo.svg
    title: Thread Safety
    details: Built-in thread pool, async logging, and smart locking for correctness in multi-threaded environments
---
