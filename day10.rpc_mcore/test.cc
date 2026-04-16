#include <iostream>

#include "UserService1.h"


int main(int argc, char const *argv[])
{

    UserService::UserService_Stub stub;

    try
    {
        int res = stub.add(1, 2);
        std::cout << "res: " << res << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    stub.print(1, 3);
    
    return 0;
}
