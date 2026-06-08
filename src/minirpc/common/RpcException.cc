#include "minirpc/common/RpcException.h"

namespace minirpc
{

RpcException::RpcException(const std::string& msg) : msg_(msg) {

}

const char* RpcException::what() const noexcept {
    return msg_.c_str();
}

} // namespace minirpc