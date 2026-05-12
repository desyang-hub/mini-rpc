#pragma once


#include "minirpc/core/RpcServer.h"
#include "minirpc/core/RpcClient.h"

#include "user.pb.h"

#include <iostream>
#include <unordered_map>

// 暂时未实现使用group_name来区分同名服务实例，所以暂时只通过服务名来区分服务实例

class UserServiceProtobuf
{
private:
    std::unordered_map<std::string, std::string> usersMap_;

public:
    std::string login(const example::User&);


    std::string logon(const example::User&);

RPC_SERVICE_BIND(UserServiceProtobuf, login, logon);
RPC_SERVICE_STUB(UserServiceProtobuf, login, logon);
};