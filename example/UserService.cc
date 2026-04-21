#include "UserService.h"

#include "minirpc/common/RpcException.h"


RPC_SERVICE_REGISTER(UserService);


std::string UserService::login(const std::string& name, const std::string& pswd) {
    // 匹配usersMap_中是否存在这个结果
    auto it = usersMap_.find(name);
    if (it == usersMap_.end()) {
        throw minirpc::RpcException("user not exitst");
    }
    else {
        if (it->second != pswd) {
            throw minirpc::RpcException("user name or password error");
        }
        return name;
    }
    return "";
}


std::string UserService::resigest(const std::string& name, const std::string& pswd) {
    auto it = usersMap_.find(name);

    if (it != usersMap_.end()) {
        throw minirpc::RpcException("username always exitst");
    }
    else {
        usersMap_[name] = pswd;
        return name;
    }

    return "";
}