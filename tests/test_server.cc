#include "minirpc/core/RpcServer.h"
#include "minirpc/core/RpcClient.h"
#include <thread>
#include <unistd.h>
#include <iostream>
#include <string>

using namespace minirpc;


class TestService {
public:
    int add(const int a, const int b) const { return a + b; }
    std::string echo(const std::string& s) const { return s; }
    int square(const int a) const { return a * a; }

    RPC_SERVICE_BIND(TestService, add, echo, square);
    RPC_SERVICE_STUB(TestService, add, echo, square);
};

RPC_SERVICE_REGISTER(TestService);


int main(int argc, char const *argv[])
{
    
    std::thread server_worker([]{
        RpcServer::GetInstance().Start(8083);
    });

    sleep(1);


    // 调用
    TestService::TestService_Stub stub;
    
    int sum = stub.add(1, 2);
    std::cout << "sum: " << sum << std::endl;
    int pow2 = stub.square(12);
    std::cout << pow2 << std::endl;


    // 停止服务
    RpcServer::GetInstance().Stop();

    std::cout << "RpcServer Stop" << std::endl;

    if (server_worker.joinable()) {
        server_worker.join();
    }

    return 0;
}
