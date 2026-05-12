#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/protocol/JsonSerialize.h"
#include "minirpc/protocol/ProtobufSerialize.h"

#include <google/protobuf/message.h>

#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t

namespace {
    minirpc::JsonSerialize& json_serializer_ =
    minirpc::JsonSerialize::GetInstance();
    minirpc::ProtobufSerialize& protobuf_serializer_ =
    minirpc::ProtobufSerialize::GetInstance();
}

namespace minirpc
{

class Serialize
{
private:
    // static JsonSerialize json_serializer_;
    // static ProtobufSerialize protobuf_serializer_;

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
            return protobuf_serializer_.deserialization<T>(data);;
        } else {
            return json_serializer_.deserialization<T>(data);
        }
    }
};


} // namespace minirpc