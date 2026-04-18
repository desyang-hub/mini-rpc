#pragma once

#include "minirpc/protocol/Protocol.h"
#include "minirpc/protocol/JsonSerialize.h"
#include "minirpc/protocol/ProtobufSerialize.h"

#include <iostream>

namespace minirpc
{

class Serialize
{
private:
    static JsonSerialize json_serializer_;
    static ProtobufSerialize protobuf_serializer_;
    static uint8_t seriType_;
public:
    static void SetSerializeType(uint8_t seriType) {
        seriType_ = SERIALIZE_JSON;
    }

    template<class T>
    static std::string Serialization(const T& obj) {
        switch (seriType_)
        {
        case SERIALIZE_JSON:
            return json_serializer_.serialization(obj);
            break;

        case SERIALIZE_PROTOBUF:
            return protobuf_serializer_.serialization(obj);
            break;
        
        default:
            throw std::runtime_error("Serializa type " + std::to_string(seriType_) + " Unsupport." );
            break;
        }
    }

    template<class T>
    static T Deserialization(const std::string& data) {
        switch (seriType_)
        {
            case SERIALIZE_JSON:
                return json_serializer_.deserialization<T>(data);
                break;

            case SERIALIZE_PROTOBUF:
                return protobuf_serializer_.deserialization<T>(data);
                break;
            
            default:
                throw std::runtime_error("Deserialization type " + std::to_string(seriType_) + " Unsupport." );
                break;
        }
    }
};


} // namespace minirpc