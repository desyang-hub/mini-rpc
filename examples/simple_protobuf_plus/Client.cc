#include "UserService.h"
#include "UserServiceJson.h" // 这个头文件直接来自simple_plus中的头文件，直接使用，无需cpp文件
#include "user.pb.h"

using namespace example;

int main(int argc, char const *argv[])
{
    UserServiceProtobuf::UserServiceProtobuf_Stub stub;

    User user;
    user.set_name("root");
    user.set_pass("root");
    // std::cout << user.SerializeAsString() << std::endl;
    // std::cout << minirpc::Serialize::Serialization(user) << std::endl;

    try
    {
        std::string msg = stub.login(user);
        if (msg == user.name())
            std::cout << "login success" << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "login error: " << e.what() << '\n';
    }


    try
    {
        stub.logon(user);
        std::cout << "register success." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    try
    {
        std::string res = stub.login(user);
        std::cout << "user " << res << " login success." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    //=======================================================================
    // UserService服务实例
    UserService::UserService_Stub jsonStub;

    try
    {
        std::string msg = jsonStub.login("root", "root");
    }
    catch(const std::exception& e)
    {
        std::cerr << "login error: " << e.what() << '\n';
    }


    try
    {
        jsonStub.logon("root", "root");
        std::cout << "register success." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    try
    {
        std::string user = jsonStub.login("root", "root");
        std::cout << "user " << user << " login success." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
