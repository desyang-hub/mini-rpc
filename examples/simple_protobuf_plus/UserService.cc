#include "UserService.h"

#include "minirpc/common/RpcException.h"


RPC_SERVICE_REGISTER(UserServiceProtobuf);


std::string UserServiceProtobuf::login(const example::User& user) {
    std::string name = user.name();
    std::string pswd = user.pass();
    // 匹配usersMap_中是否存在这个结果
    auto it = usersMap_.find(name);
    if (it == usersMap_.end()) {
        throw minirpc::RpcException("user not exitst");
    }
    else {
        if (it->second != pswd) {
            throw minirpc::RpcException("user name or password error");
        }
        std::cout << "login success." << std::endl;
        return name;
    }
    return "";
}


std::string UserServiceProtobuf::logon(const example::User& user) {

    std::string name = user.name();
    std::string pswd = user.pass();

    auto it = usersMap_.find(name);

    if (it != usersMap_.end()) {
        throw minirpc::RpcException("username always exitst");
    }
    else {
        usersMap_[name] = pswd;
        std::cout << "register success." << std::endl;
        return name;
    }

    return "";
}