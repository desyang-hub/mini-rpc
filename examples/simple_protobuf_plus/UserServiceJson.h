/**
 * @FilePath     : /mini-rpc/examples/simple_protobuf_plus/UserServiceJson.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-10 16:17:06
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-10 16:17:13
**/
#pragma once

// #include "minirpc/core/RpcServer.h"
// #include "minirpc/core/RpcClient.h"

#include "minirpc/rpc_core/RpcServer.h"
#include "minirpc/rpc_core/RpcClient.h"

#include <iostream>
#include <unordered_map>

class UserService
{
private:
    std::unordered_map<std::string, std::string> usersMap_;

public:
    std::string login(const std::string &name, const std::string &pswd);

    std::string logon(const std::string &name, const std::string &pswd);

private:
    RPC_SERVICE_BIND(UserService, login, logon);
    RPC_SERVICE_STUB(UserService, login, logon);
};