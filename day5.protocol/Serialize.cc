#include "Serialize.h"
#include "JsonSerialize.h"
#include "ProtobufSerialize.h"

#include <stdexcept>

JsonSerialize Serialize::json_serializer_;
ProtobufSerialize Serialize::protobuf_serializer_;
uint8_t Serialize::seriType_ = SERIALIZE_JSON;