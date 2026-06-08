#pragma once

#include <stdexcept>
#include <string>

namespace minirpc
{
    
class RpcException : public std::exception {
private:
    const std::string msg_;
public:
    RpcException(const std::string& msg);

    const char* what() const noexcept override;
};


} // namespace minirpc