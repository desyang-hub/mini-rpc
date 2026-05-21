#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/protocol/JsonSerialize.h"
#include "minirpc/protocol/ProtobufSerialize.h"

#include <google/protobuf/message.h>

#include <cstdint>
#include <cstddef>

namespace minirpc
{

class Serialize
{
private:
    static inline JsonSerialize& json_serializer_ = JsonSerialize::GetInstance();
    static inline ProtobufSerialize& protobuf_serializer_ = ProtobufSerialize::GetInstance();

public:
    template<class T>
    static std::string Serialization(const T& obj) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
            return protobuf_serializer_.serialization(obj);
        } else {
            return json_serializer_.serialization(obj);
        }
    }

    template<class T>
    static T Deserialization(const std::string& data) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
            return protobuf_serializer_.deserialization<T>(data);
        } else {
            return json_serializer_.deserialization<T>(data);
        }
    }
};


} // namespace minirpc
