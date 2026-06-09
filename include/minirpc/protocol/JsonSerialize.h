#pragma once

#include "nlohmann/json.hpp"
#include <cstdint>

namespace minirpc
{


class JsonSerialize
{
private:
    JsonSerialize() {}

public:
    static JsonSerialize& GetInstance() {
        static JsonSerialize instance_;
        return instance_;
    }

    // 序列化接口
    /// @param body: 待序列化的数据
    template<class T>
    std::string serialization(const T& obj) {
        return nlohmann::json(obj).dump();
    }

    // 反序列化接口
    /// @param bytes: 序列化后的数据
    /// @return 还原后的数据
    template<class T>
    T deserialization(const std::string& serializeStr) {
        return nlohmann::json::parse(serializeStr).get<T>();
    }

    // 反序列化接口
    /// @param bytes: 序列化后的数据
    /// @return 还原后的数据
    template<class T>
    T deserialization(const void* serializeStr, size_t len) {
        return nlohmann::json::parse((const char*)serializeStr, (const char*)serializeStr + len).get<T>();
    }
};


} // namespace minirpc