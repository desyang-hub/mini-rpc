#include "minirpc/core/RpcServer.h"

namespace minirpc
{


std::unordered_map<std::string, RpcHandler> RpcServer::handlers_;


} // namespace minirpc