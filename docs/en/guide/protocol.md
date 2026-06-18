# Protocol Specification

mini-rpc uses a custom binary protocol for communication. The protocol header is fixed at **27 bytes** and supports variable-length body.

## Protocol Header Structure

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

## Field Description

| Field | Type | Bytes | Description |
|-------|------|-------|-------------|
| `magic` | `uint16_t` | 2 | Magic number `0x5250` ("RP"), used for protocol identification |
| `version` | `uint8_t` | 1 | Protocol version, currently `1` |
| `type` | `uint8_t` | 1 | Message type: `1`=Request, `2`=Response, `3`=Heartbeat |
| `serialize` | `uint8_t` | 1 | Serialization format: `1`=JSON, `2`=Protobuf |
| `compress` | `uint8_t` | 1 | Compression: `0`=None |
| `request_id` | `uint64_t` | 8 | Unique request identifier for request-response correlation |
| `body_len` | `uint32_t` | 4 | Length of the body (data payload) |
| `checksum` | `uint32_t` | 4 | CRC32 checksum |
| `srv_name_len` | `uint32_t` | 4 | Length of the service name |
| `code` | `uint8_t` | 1 | Status code: `0`=Success, `1`=Failed, `2`=Timeout |

## Complete Packet Format

Transmission order:

```
+-------------------+---------------------+-------------------+---------------+
|   ProtocolHeader  |   ServiceName       |      Body         |  check_num    |
|    (27 bytes)     | (srv_name_len bytes)|  (body_len bytes) |   (4 bytes)   |
+-------------------+---------------------+-------------------+---------------+
```

- **ProtocolHeader**: Fixed 27-byte header
- **ServiceName**: Service name in format `ClassName.methodName` (e.g. `UserService.login`), UTF-8 encoded
- **Body**: Serialized data payload, format determined by `serialize` field
- **check_num**: CRC32 checksum covering the entire `[Header | srv_name | body]` content

## CRC32 Checksum

`check_num` is at the end of the packet, computed over `[Header | srv_name | body]`:

```cpp
// Compute CRC32 over header + srv_name + body
size_t pkg_len = header_len + header.srv_name_len + len;
uint32_t check_num = simple_crc32(crc_input.data(), pkg_len);
```

Decoding validates magic number first, then computes CRC32 to verify packet integrity. Maximum body size: 64MB.

## Serialization Formats

### Automatic Type Detection

The framework automatically selects serialization based on parameter type:

- **Types derived from `google::protobuf::Message`** → Protobuf serialization
- **Other types** → JSON serialization (nlohmann/json)

```cpp
// Automatic serialization selection
std::string body = Serialize::Serialization(args_tuple);

// Automatic deserialization selection
auto result = Serialize::Deserialization<T>(body);
```

### JSON Serialization

Uses [nlohmann/json](https://github.com/nlohmann/json) for serialization/deserialization. Supports basic types, `std::string`, `std::tuple`, `std::vector`, and other standard containers.

### Protobuf Serialization

Protobuf is integrated and ready to use. Automatically selected when parameter type is a protobuf Message derived class.

## Encoding & Decoding

### Encoder

```cpp
// Request encoding
Bytes EncodeReq(uint64_t id, const char* name, const void* data, size_t len);

// Success response encoding
Bytes SuccessRes(uint64_t request_id, const void* data, size_t len);

// Error response encoding
Bytes ErrorRes(uint64_t request_id, uint8_t errcode, const char* errmsg);
```

Assembles Header, ServiceName, and Body, then computes check_num.

### Decoder

```cpp
// Validate packet integrity
int Decode(const void* data, int len);
// Returns: ERR(-1) | UN_FINISH(0) | packet_length(>0)

// Decode to Response
int Decode(const void* data, Response& resp);
// Returns: request_id
```

Decoding flow:
1. Check data length sufficiency
2. Validate magic number `magic == 0x5250`
3. Compute CRC32 and verify check_num
4. Extract header, service name, and body

## Message Types

| Value | Name | Description |
|-------|------|-------------|
| 1 | MSG_REQUEST | RPC request initiated by client |
| 2 | MSG_RESPONSE | RPC response returned by server |
| 3 | MSG_HEARTBEAT | Keep-alive heartbeat message |

## Status Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | SUCCESS | Call successful |
| 1 | FAILED | Call failed |
| 2 | TIMEOUT | Call timed out |
