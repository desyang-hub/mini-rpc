#pragma once

#include "minirpc/protocol/JsonSerialize.h"
#include "minirpc/protocol/ProtobufSerialize.h"
#include "minirpc/common/RpcException.h"

#include <google/protobuf/message.h>

#include <cstdint>
#include <cstddef>

namespace minirpc
{

class Serialize
{
private:
    JsonSerialize& json_serializer_;
    ProtobufSerialize& protobuf_serializer_;

    Serialize() : json_serializer_(JsonSerialize::GetInstance()),
        protobuf_serializer_(ProtobufSerialize::GetInstance()) {}

    static Serialize& GetInstance() {
        static Serialize serialize;
        return serialize;
    }

    template<class T>
    std::string serialization(const T& obj) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
            return protobuf_serializer_.serialization(obj);
        } else {
            return json_serializer_.serialization(obj);
        }
    }

    template<class T>
    T deserialization(const void* data, size_t len) {
        if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
            return protobuf_serializer_.deserialization<T>(data, len);
        } else {
            return json_serializer_.deserialization<T>(data, len);
        }
    }

public:
    template<class T>
    static std::string Serialization(const T& obj) {
        return GetInstance().serialization(obj);
    }

    template<class T>
    static T Deserialization(const std::string& data) {
        return GetInstance().deserialization<T>(data.c_str(), data.size());
    }

    template<class T>
    static T Deserialization(const void* data, size_t len) {
        return GetInstance().deserialization<T>(data, len);
    }
};


} // namespace minirpc
