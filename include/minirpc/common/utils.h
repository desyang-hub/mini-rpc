#pragma once

#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
namespace minirpc
{

// 用于计算crc校验结果， checknum计算
uint32_t simple_crc32(const uint8_t *data, size_t len);

} // namespace minirpc