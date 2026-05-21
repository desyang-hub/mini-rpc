#pragma once

#include <cstdint>
#include <string>

namespace minirpc {

struct Response
{
    uint8_t state;
    std::string data;
};

} // namespace minirpc
