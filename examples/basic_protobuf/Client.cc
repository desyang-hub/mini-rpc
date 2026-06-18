#include "UserService.h"
#include "minirpc/common/Config.h"
#include "user.pb.h"

#include <iostream>

int main()
{
    auto cfg = minirpc::loadConfig();
    minirpc::RpcClient::GetInstance().init(cfg.registry_address);

    UserServiceProtobuf::UserServiceProtobuf_Stub stub;

    example::User user;
    user.set_name("bob");
    user.set_pass("pass123");

    // Register user
    try {
        stub.logon(user);
        std::cout << "[client] user 'bob' registered" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[client] logon error: " << e.what() << std::endl;
    }

    // Login
    try {
        std::string msg = stub.login(user);
        std::cout << "[client] login: " << msg << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[client] login error: " << e.what() << std::endl;
    }

    return 0;
}
