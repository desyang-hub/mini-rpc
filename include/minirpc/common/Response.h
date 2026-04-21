#pragma once

#include <iostream>
#include <cstdint>

struct Response
{
    uint8_t state;
    std::string data;
};
