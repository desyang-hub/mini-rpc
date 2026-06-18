#pragma once

#include "minirpc/protocol/JsonSerialize.h"
#include "minirpc/protocol/ProtobufSerialize.h"

#include <google/protobuf/message.h>

#include <cstddef>
#include <cstdint>

namespace minirpc
{

// Auto-detects protobuf vs JSON based on message type
class Serialize
{
private:
    JsonSerialize& json_;
    ProtobufSerialize& protobuf_;

    Serialize() : json_(JsonSerialize::GetInstance()), protobuf_(ProtobufSerialize::GetInstance()) {}

    template<class T>
    std::string doSerialize(const T& obj)
    {
        if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
            return protobuf_.serialization(obj);
        } else {
            return json_.serialization(obj);
        }
    }

    template<class T>
    T doDeserialize(const void* data, size_t len)
    {
        if constexpr (std::is_base_of_v<google::protobuf::Message, T>) {
            return protobuf_.deserialization<T>(data, len);
        } else {
            return json_.deserialization<T>(data, len);
        }
    }

public:
    template<class T>
    static std::string Serialization(const T& obj)
    {
        static Serialize inst;
        return inst.doSerialize(obj);
    }

    template<class T>
    static T Deserialization(const std::string& data)
    {
        static Serialize inst;
        return inst.doDeserialize<T>(data.c_str(), data.size());
    }

    template<class T>
    static T Deserialization(const void* data, size_t len)
    {
        static Serialize inst;
        return inst.doDeserialize<T>(data, len);
    }
};

} // namespace minirpc
