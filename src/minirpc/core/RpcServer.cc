#include "minirpc/core/RpcServer.h"

namespace minirpc
{
    void RpcServer::registerService(const std::string& clsNmae) {
        LOG_INFO("regis name %s", clsNmae.c_str());
        std::unique_lock<std::shared_mutex> lock(hanelers_mutex_);
        services_.push_back(clsNmae);
    }

    bool RpcServer::call(const std::string &name, const std::string &body, std::string &res)
    {
        std::unordered_map<std::string, RpcHandler>::iterator it{};
        {
            // 使用读锁
            std::shared_lock<std::shared_mutex> lock(hanelers_mutex_);
            it = handlers_.find(name);
        }

        if (it == handlers_.end())
        {
            LOG_ERROR("Method not support %s", name.c_str());
            // std::cout << "Method not support " << name << std::endl;
            res = "Method not support";
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
            res = e.what();
            return false;
        }

        return true;
    }


} // namespace minirpc