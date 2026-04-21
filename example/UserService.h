#pragma once


#include "minirpc/core/RpcServer.h"
#include "minirpc/core/RpcClient.h"

#include <iostream>
#include <unordered_map>

class UserService
{
private:
    std::unordered_map<std::string, std::string> usersMap_;

public:
    std::string login(const std::string& name, const std::string& pswd);


    std::string resigest(const std::string& name, const std::string& pswd);

RPC_SERVICE_BIND(UserService, login, resigest);
RPC_SERVICE_STUB(UserService, login, resigest);
};