#include <iostream>

#include "RpcClient.h"


int main(int argc, char const *argv[])
{

    RpcClient cli;

    int result;
    bool is_success = cli.call<int>("UserService.add", std::tuple<int, int>{1, 2}, result);

    if (is_success) {
        std::cout << "Result: " << result << std::endl;
    }
    else {
        std::cout << "Call failed" << std::endl;
    }

    is_success = cli.call<int>("UserService.add", std::tuple<int>{1}, result);

    if (is_success) {
        std::cout << "Result: " << result << std::endl;
    }
    else {
        std::cout << "Call failed" << std::endl;
    }


    return 0;
}

