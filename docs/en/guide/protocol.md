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
| `type` | `uint8_t` | 1 | Message type: `0`=Request, `1`=Response, `2`=Heartbeat |
| `serialize` | `uint8_t` | 1 | Serialization format: `1`=JSON, `2`=Protobuf |
| `compress` | `uint8_t` | 1 | Compression: `0`=None |
| `request_id` | `uint64_t` | 8 | Unique request identifier for request-response correlation |
| `body_len` | `uint32_t` | 4 | Length of the body (data payload) |
| `checksum` | `uint32_t` | 4 | CRC32 checksum computed over the body |
| `srv_name_len` | `uint32_t` | 4 | Length of the service name |
| `code` | `uint8_t` | 1 | Status code: `0`=Success, others are error codes |

## Complete Packet Format

```
+-------------------+---------------------+-------------------+
|   ProtocolHeader  |   ServiceName       |      Body         |
|    (27 bytes)     | (srv_name_len bytes)|  (body_len bytes) |
+-------------------+---------------------+-------------------+
```

- **ProtocolHeader**: Fixed 27-byte header
- **ServiceName**: Service name in format `ClassName.methodName` (e.g. `UserService.login`), UTF-8 encoded
- **Body**: Serialized data payload, format determined by `serialize` field

## CRC32 Checksum

The CRC32 checksum is computed over the **Body** portion:

```cpp
uint32_t checksum = minirpc::simple_crc32(body.data(), body.size());
```

The server validates the magic number and CRC32 upon receiving a packet. Invalid packets are discarded.

## Serialization Formats

### JSON (Default)

Uses [nlohmann/json](https://github.com/nlohmann/json) v3.11.2 for serialization/deserialization:

```cpp
// Serialize
std::string body = Serialize::Serialization(args_tuple);

// Deserialize
auto result = Serialize::Deserialization<T>(body);
```

### Protobuf (Reserved)

Interface defined but not implemented:

```cpp
Serialize::SetSerializeType(SERIALIZE_PROTOBUF); // Throws "Protobuf not supported" at runtime
```

## Encoding & Decoding

### Encoder::Encode

Assembles a complete packet from service name and serialized body:

```
Header(27B) + ServiceName(N bytes) + Body(M bytes)
```

Computes CRC32 checksum and writes it to the header's `checksum` field.

### Decoder::Decode

Parses a packet from raw byte stream:

1. Validates magic number `magic == 0x5250`
2. Validates length consistency
3. Verifies CRC32 checksum
4. Extracts header, service name, and body

## Message Types

| Value | Name | Description |
|-------|------|-------------|
| 0 | Request | RPC request initiated by client |
| 1 | Response | RPC response returned by server |
| 2 | Heartbeat | Keep-alive heartbeat message |

## Status Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | SUCCESS | Call successful |
| 1-255 | ERROR | Call failed, specific error code defined by server |
