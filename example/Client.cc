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
        std::cout << "Exception Info: " << std::endl;
        std::cerr << e.what() << '\n';
    }

    try
    {
        stub.resigest("root", "root");
        std::string msg = stub.login("root", "root");

        std::cout << msg << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


    return 0;
}
