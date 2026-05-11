#pragma once

#include <iostream>
#include <google/protobuf/message.h>

namespace minirpc
{


class ProtobufSerialize
{
private:
    ProtobufSerialize() {};

public:
    static ProtobufSerialize& GetInstance() {
        static ProtobufSerialize instance_;
        return instance_;
    }

    // 序列化接口
    // 序列化：要求 ProtobufMessageInherit 是 protobuf message 类型
    /// @param body: 待序列化的数据
    template<class ProtobufMessageInherit>
    std::string serialization(const ProtobufMessageInherit& obj) {
        static_assert(std::is_base_of_v<google::protobuf::Message, ProtobufMessageInherit>, "ProtobufMessageInherit must inherit from google::protobuf::Message");

        std::string output;
        if (!obj.SerializeToString(&output)) {
            throw std::runtime_error("Protobuf serialization failed");
        }
        return output;
    }

    // 反序列化接口
    /// @param bytes: 序列化后的数据
    /// @return 还原后的数据
    template<class ProtobufMessageInherit>
    ProtobufMessageInherit deserialization(const std::string& serializeStr) {
        static_assert(std::is_base_of_v<google::protobuf::Message, ProtobufMessageInherit>, "ProtobufMessageInherit must inherit from google::protobuf::Message");

        ProtobufMessageInherit obj;
        if (!obj.ParseFromString(serializeStr)) {
            throw std::runtime_error("Protobuf deserialization failed");
        }
        return obj;
    }
};


} // namespace minirpc