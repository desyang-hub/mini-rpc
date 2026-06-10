/**
 * @FilePath     : /mini-rpc/src/minirpc/net/EndPoint.cc
 * @Description  : EndPoint implementation
 * @Author       : desyang
 * @Date         : 2026-06-10 11:48:37
**/
#include "minirpc/net/EndPoint.h"
#include <functional>
#include <cstdint>

namespace minirpc
{

bool EndPoint::operator==(const EndPoint &rhs) const
{
    return port == rhs.port && host == rhs.host;
}

EndPoint::EndPoint(const std::string &host, int port) : port(port), host(host) {}

} // namespace minirpc