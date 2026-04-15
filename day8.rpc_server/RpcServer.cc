#include "RpcServer.h"

std::unordered_map<std::string, Handler> RpcServer::handlers_;