#include "UserService.h"

#include "minirpc/common/RpcException.h"


RPC_SERVICE_REGISTER(UserServiceProtobuf);


std::string UserServiceProtobuf::login(const example::User& user) {
    std::string name = user.name();
    std::string pswd = user.pass();
    // 匹配usersMap_中是否存在这个结果
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = usersMap_.find(name);
    if (it == usersMap_.end()) {
        LOG_INFO("name count: %lu", usersMap_.count(name));
        throw minirpc::RpcException("user " + user.name() + " not exitst");
    }
    else {
        if (it->second != pswd) {
            throw minirpc::RpcException("user name or password error");
        }
        // std::cout << "login success." << std::endl;
        return name;
    }
    return "";
}


std::string UserServiceProtobuf::logon(const example::User& user) {

    std::string name = user.name();
    std::string pswd = user.pass();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = usersMap_.find(name);

    if (it != usersMap_.end()) {
        throw minirpc::RpcException("username always exitst");
    }
    else {
        usersMap_[name] = pswd;
        // std::cout << "register success." << std::endl;
        return name;
    }

    return "";
}


int UserServiceProtobuf::add(int a, int b) {
    return a + b;
}

int UserServiceProtobuf::sub(int a, int b) {
    return a - b;
}