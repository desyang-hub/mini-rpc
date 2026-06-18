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
| `type` | `uint8_t` | 1 | 消息类型：`1`=请求, `2`=响应, `3`=心跳 |
| `serialize` | `uint8_t` | 1 | 序列化方式：`1`=JSON, `2`=Protobuf |
| `compress` | `uint8_t` | 1 | 压缩方式：`0`=无压缩 |
| `request_id` | `uint64_t` | 8 | 请求唯一标识，用于请求-响应关联 |
| `body_len` | `uint32_t` | 4 | 数据体（body）长度 |
| `checksum` | `uint32_t` | 4 | CRC32 校验码 |
| `srv_name_len` | `uint32_t` | 4 | 服务名长度 |
| `code` | `uint8_t` | 1 | 状态码：`0`=成功, `1`=失败, `2`=超时 |

## 完整数据包格式

传输顺序为：

```
+-------------------+---------------------+-------------------+---------------+
|   ProtocolHeader  |   ServiceName       |      Body         |  check_num    |
|    (27 bytes)     | (srv_name_len bytes)|  (body_len bytes) |   (4 bytes)   |
+-------------------+---------------------+-------------------+---------------+
```

- **ProtocolHeader**: 固定 27 字节头部
- **ServiceName**: 服务名，格式为 `ClassName.methodName`（如 `UserService.login`），UTF-8 编码
- **Body**: 序列化后的数据体，内容由 `serialize` 字段决定
- **check_num**: CRC32 校验值，覆盖 `[Header | srv_name | body]` 全部内容

## CRC32 校验

`check_num` 位于数据包末尾，对 `[Header | srv_name | body]` 全部内容计算 CRC32：

```cpp
// 计算 CRC32，覆盖 header + srv_name + body
size_t pkg_len = header_len + header.srv_name_len + len;
uint32_t check_num = simple_crc32(crc_input.data(), pkg_len);
```

解码时先校验魔术字，再计算 CRC32 验证数据包完整性。最大包体限制：64MB。

## 序列化格式

### 自动类型检测

框架根据参数类型自动选择序列化方式：

- **继承自 `google::protobuf::Message`** → Protobuf 序列化
- **其他类型** → JSON 序列化 (nlohmann/json)

```cpp
// 自动选择序列化方式
std::string body = Serialize::Serialization(args_tuple);

// 自动选择反序列化方式
auto result = Serialize::Deserialization<T>(body);
```

### JSON 序列化

使用 [nlohmann/json](https://github.com/nlohmann/json) 进行序列化/反序列化。支持基本类型、`std::string`、`std::tuple`、`std::vector` 等标准容器。

### Protobuf 序列化

Protobuf 已集成可用，参数类型为 protobuf Message 派生类时自动使用。

## 编码与解码

### Encoder

```cpp
// 请求编码
Bytes EncodeReq(uint64_t id, const char* name, const void* data, size_t len);

// 成功响应编码
Bytes SuccessRes(uint64_t request_id, const void* data, size_t len);

// 错误响应编码
Bytes ErrorRes(uint64_t request_id, uint8_t errcode, const char* errmsg);
```

将 Header、ServiceName、Body 组装并计算 check_num。

### Decoder

```cpp
// 校验包完整性
int Decode(const void* data, int len);
// 返回: ERR(-1) | UN_FINISH(0) | 包长度(>0)

// 解码为 Response
int Decode(const void* data, Response& resp);
// 返回: request_id
```

解码流程：
1. 检查数据长度是否足够
2. 验证魔术字 `magic == 0x5250`
3. 计算 CRC32 并验证 check_num
4. 提取 header、服务名、body

## 消息类型

| 类型值 | 名称 | 说明 |
|--------|------|------|
| 1 | MSG_REQUEST | 客户端发起的 RPC 请求 |
| 2 | MSG_RESPONSE | 服务端返回的 RPC 响应 |
| 3 | MSG_HEARTBEAT | 心跳保活消息 |

## 状态码

| 码值 | 名称 | 说明 |
|------|------|------|
| 0 | SUCCESS | 调用成功 |
| 1 | FAILED | 调用失败 |
| 2 | TIMEOUT | 调用超时 |
