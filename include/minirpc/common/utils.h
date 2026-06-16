#pragma once

#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <future>

namespace minirpc
{

// 用于计算crc校验结果， checknum计算
uint32_t simple_crc32(const void* ptr, size_t len);

template<typename R>
R get_with_timeout(std::future<R>& fut, std::chrono::milliseconds timeout) {
    auto status = fut.wait_for(timeout);
    
    if (status == std::future_status::ready) {
        return fut.get();
    } else if (status == std::future_status::timeout) {
        throw std::runtime_error("Future timed out!");
    } else {
        throw std::runtime_error("Future is deferred!");
    }
}

} // namespace minirpc