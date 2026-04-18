#include "minirpc/core/RpcServer.h"

namespace minirpc
{


bool RpcServer::call(const std::string &name, const std::string &body, std::string &res)
{
    auto& handlers_ = GetHandlers();
    auto it = handlers_.find(name);

    if (it == handlers_.end())
    {
        LOG_ERROR("Method not support %s", name.c_str());
        // std::cout << "Method not support " << name << std::endl;
        return false;
    }

    try
    {
        it->second(body, res);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("%s", e.what());
        // std::cerr << e.what() << '\n';
        return false;
    }

    return true;
}


} // namespace minirpc