#pragma once

#include <stdexcept>
#include <string>

class RpcException : public std::runtime_error {
public:
    RpcException(const std::string& msg) : std::runtime_error(msg) {}
};