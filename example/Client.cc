#include "UserService.h"

int main(int argc, char const *argv[])
{
    UserService::UserService_Stub stub;

    try
    {
        std::string msg = stub.login("root", "root");
    }
    catch(const std::exception& e)
    {
        std::cerr << "login error: " << e.what() << '\n';
    }


    try
    {
        stub.logon("root", "root");
        std::cout << "register success." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    try
    {
        std::string user = stub.login("root", "root");
        std::cout << "user " << user << " login success." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


    return 0;
}
