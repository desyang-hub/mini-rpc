#pragma once

#include "nlohmann/json.hpp"

class JsonSerialize
{

public:
    // 序列化接口
    /// @param body: 待序列化的数据
    template<class T>
    std::string serialization(T& obj) {
        return nlohmann::json(obj).dump();
    }

    // 反序列化接口
    /// @param bytes: 序列化后的数据
    /// @return 还原后的数据
    template<class T>
    T deserialization(const std::string& serializeStr) {
        return nlohmann::json::parse(serializeStr).get<T>();
    }
};
