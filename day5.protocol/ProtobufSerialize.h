#pragma once

#include <iostream>

class ProtobufSerialize
{
public:
    // 序列化接口
    /// @param body: 待序列化的数据
    template<class T>
    std::string serialization(T& obj) {
        throw std::runtime_error("Method not support");
    }

    // 反序列化接口
    /// @param bytes: 序列化后的数据
    /// @return 还原后的数据
    template<class T>
    T deserialization(const std::string& serializeStr) {
        throw std::runtime_error("Method not support");
    }
};