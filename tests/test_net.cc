#include <gtest/gtest.h>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>
#include "nlohmann/json.hpp"

#include "minirpc/core/RpcServer.h"
#include "minirpc/core/RpcClient.h"
#include "minirpc/net/TcpServer.h"

using namespace std;
using namespace minirpc;
using json = nlohmann::json;

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


TEST(UNetTest, TestTcpServer) {
    
}


// 启用前确保nacos已经启用
// TEST(UNetTest, TestTcpServer) {

//     TcpServer server;   
//     std::thread server_worker([s = std::move(server)]() mutable {
//         s.serve(8850);
//     });
//     server_worker.detach();

//     // 启用客户端
//     // std::thread cli_loop(&RpcClient::ReadLoop, &RpcClient::GetInstance());

//     // cli_loop.detach();

//     // 等待服务启动
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));

//     TestService::TestService_Stub stub;

//     int res = stub.add(1, 2);

//     EXPECT_EQ(res, 3);

//     res = stub.sub(1, 2);

//     EXPECT_EQ(res, -1);
// }