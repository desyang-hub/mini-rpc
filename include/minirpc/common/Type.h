#pragma once

#include <iostream>
#include <cstdint>   // 添加这行
#include <cstddef>   // 可选，提供 size_t
#include <vector>
#include <functional>


namespace minirpc
{

using Bytes = std::vector<uint8_t>;
using RpcHandler = std::function<void(const std::string&, std::string&)>;
    
} // namespace minirpc
