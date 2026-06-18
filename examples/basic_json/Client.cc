#include "UserService.h"
#include "minirpc/common/Config.h"

#include <iostream>

int main()
{
    auto cfg = minirpc::loadConfig();
    minirpc::RpcClient::GetInstance().init(cfg.registry_address);

    UserService::UserService_Stub stub;

    // Register user first
    try {
        stub.logon("alice", "secret");
        std::cout << "[client] user 'alice' registered" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[client] logon error: " << e.what() << std::endl;
    }

    // Login
    try {
        std::string msg = stub.login("alice", "secret");
        std::cout << "[client] login: " << msg << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[client] login error: " << e.what() << std::endl;
    }

    return 0;
}
