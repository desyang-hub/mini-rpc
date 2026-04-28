#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"

#include "minirpc/common/utils.h"
#include "minirpc/common/logger.h"

#include "minirpc/core/RpcClient.h"
#include "minirpc/core/RpcServer.h"
#include "minirpc/core/IConnection.h"

#include "minirpc/net/TcpServer.h"
#include "minirpc/net/TcpClient.h"

using namespace minirpc;


class TestService
{
public:
    int add(const int a, const int b) const {
        return a + b;
    }

    int sub(const int a, const int b) const {
        return a - b;
    }

RPC_SERVICE_BIND(TestService, add, sub);
RPC_SERVICE_STUB(TestService, add, sub);
};

RPC_SERVICE_REGISTER(TestService);


int main(int argc, char const *argv[])
{

    // Logger::GetInstanse().setLevel(DEBUG);
    // // ENABLE_ASYNC_LOGING();

    // // 获取服务
    // auto ip = getServiceAddress("UserService");
    // LOG_INFO("server ip: %s", ip.c_str());


    TcpServer server;   
    std::thread server_worker([s = std::move(server)]() mutable {
        s.serve(8849);
    });
    server_worker.detach();

    // 启用客户端
    // std::thread cli_loop(&RpcClient::ReadLoop, &RpcClient::GetInstance());

    // cli_loop.detach();

    // 等待服务启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    TestService::TestService_Stub stub;

    int res = stub.add(1, 2);

    std::cout << "sum: " << res << std::endl;

    res = stub.sub(1, 2);

    std::cout << "sub: " << res << std::endl;
    
    return 0;
}
