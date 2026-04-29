# 协议规范

mini-rpc 使用自定义二进制协议进行通信，协议头固定为 **27 字节**，支持变长数据体。

## 协议头结构

```
+--------+--------+--------+--------+--------+
| magic  | ver    | type   | serialize| compress|
| 2 byte |  1B    |  1B    |   1B    |  1B    |
+--------+--------+--------+--------+--------+
|          request_id (uint64)               |
|              8 bytes                       |
+--------+--------+--------+--------+--------+
|          body_len (uint32)                 |
|              4 bytes                       |
+--------+--------+--------+--------+--------+
|          checksum (CRC32, uint32)          |
|              4 bytes                       |
+--------+--------+--------+--------+--------+
|   srv_name_len (uint32)   |   code  |
|         4 bytes           |  1B   |
+--------+--------+--------+--------+--------+
```

## 字段说明

| 字段 | 类型 | 字节数 | 说明 |
|------|------|--------|------|
| `magic` | `uint16_t` | 2 | 魔术字 `0x5250`（"RP"），用于协议识别 |
| `version` | `uint8_t` | 1 | 协议版本号，当前为 `1` |
| `type` | `uint8_t` | 1 | 消息类型：`0`=请求, `1`=响应, `2`=心跳 |
| `serialize` | `uint8_t` | 1 | 序列化方式：`1`=JSON, `2`=Protobuf |
| `compress` | `uint8_t` | 1 | 压缩方式：`0`=无压缩 |
| `request_id` | `uint64_t` | 8 | 请求唯一标识，用于请求-响应关联 |
| `body_len` | `uint32_t` | 4 | 数据体（body）长度 |
| `checksum` | `uint32_t` | 4 | CRC32 校验码，对 body 计算 |
| `srv_name_len` | `uint32_t` | 4 | 服务名长度 |
| `code` | `uint8_t` | 1 | 状态码：`0`=成功, 其他为错误码 |

## 完整数据包格式

```
+-------------------+---------------------+-------------------+
|   ProtocolHeader  |   ServiceName       |      Body         |
|    (27 bytes)     | (srv_name_len bytes)|  (body_len bytes) |
+-------------------+---------------------+-------------------+
```

- **ProtocolHeader**: 固定 27 字节头部
- **ServiceName**: 服务名，格式为 `ClassName.methodName`（如 `UserService.login`），UTF-8 编码
- **Body**: 序列化后的数据体，内容由 `serialize` 字段决定

## CRC32 校验

CRC32 校验码对 **Body 部分**计算：

```cpp
uint32_t checksum = minirpc::simple_crc32(body.data(), body.size());
```

服务端收到数据包后先验证 magic 号和 CRC32，校验失败则丢弃该包。

## 序列化格式

### JSON (默认)

使用 [nlohmann/json](https://github.com/nlohmann/json) v3.11.2 进行序列化/反序列化：

```cpp
// 序列化
std::string body = Serialize::Serialization(args_tuple);

// 反序列化
auto result = Serialize::Deserialization<T>(body);
```

### Protobuf (预留)

接口已定义但未实现：

```cpp
Serialize::SetSerializeType(SERIALIZE_PROTOBUF); // 运行时抛出 "Protobuf not supported"
```

## 编码与解码

### Encoder::Encode

将服务名和序列化后的数据体组装为完整数据包：

```
Header(27B) + ServiceName(N bytes) + Body(M bytes)
```

计算 CRC32 校验码并写入 header 的 `checksum` 字段。

### Decoder::Decode

从原始字节流中解析数据包：

1. 检查魔术字 `magic == 0x5250`
2. 检查数据长度一致性
3. 验证 CRC32 校验码
4. 提取 header、服务名、body

## 消息类型

| 类型值 | 名称 | 说明 |
|--------|------|------|
| 0 | Request | 客户端发起的 RPC 请求 |
| 1 | Response | 服务端返回的 RPC 响应 |
| 2 | Heartbeat | 心跳保活消息 |

## 状态码

| 码值 | 名称 | 说明 |
|------|------|------|
| 0 | SUCCESS | 调用成功 |
| 1-255 | ERROR | 调用失败，具体错误码由服务端定义 |
