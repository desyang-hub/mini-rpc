#include "minirpc/protocol/Serialize.h"
#include "minirpc/protocol/JsonSerialize.h"
#include "minirpc/protocol/ProtobufSerialize.h"

#include <stdexcept>

namespace minirpc
{

JsonSerialize Serialize::json_serializer_;
ProtobufSerialize Serialize::protobuf_serializer_;
uint8_t Serialize::seriType_ = SERIALIZE_JSON;

} // namespace minirpc