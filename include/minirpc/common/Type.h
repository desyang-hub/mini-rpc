#pragma once

#include <iostream>
#include <vector>
#include <functional>


namespace minirpc
{

using Bytes = std::vector<uint8_t>;
using RpcHandler = std::function<void(const std::string&, std::string&)>;
    
} // namespace minirpc
