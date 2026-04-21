#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"

#include "minirpc/common/utils.h"
#include "minirpc/common/logger.h"

#include "minirpc/core/RpcClient.h"
#include "minirpc/core/RpcServer.h"

#include "minirpc/net/TcpServer.h"

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


#include <fcntl.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>
#include "nlohmann/json.hpp"
#include <stdlib.h>

using json = nlohmann::json;

void cli_worker() {

    // sleep 3 seconds wait server lunch.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // 建立连接，并通过send 发送package
    // 1. 创建socket
    int clifd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 目标addr
    std::string host = "127.0.0.1";
    int port = 8080;
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(clifd, (sockaddr*)&server_addr, sizeof server_addr) < 0) {
        perror("connect error");
        close(clifd);
        exit(-1);
    }

    std::tuple<int, int> param{1, 2};
    json obj = param;
    std::string body = obj.dump();
    auto bytes = Encoder::Encode("TestService.add", body);
    

    if (send(clifd, bytes.data(), bytes.size(), 0) == -1) {
        LOG_ERROR("send msg error");
        return;
    }

    // 接收结果
    RingBuffer buf;
    int save_errno;
    int len = buf.read_fd(clifd, &save_errno);
    if (len == -1) {
        LOG_FATAL("recv msg error");
    }
    

    int bytesRead = buf.readable_bytes();
    std::vector<uint8_t> data(bytesRead);

    buf.peek_data(static_cast<void*>(data.data()), bytesRead);

    LOG_INFO("res: %s", reinterpret_cast<char*>(data.data()));

    // ProtocolHeader header;
    // std::string res;
    // Decoder::Decode(data, header, res);

    // LOG_INFO("res: %s", res.c_str());

    close(clifd);
}

int main(int argc, char const *argv[])
{

    Logger::GetInstanse().setLevel(DEBUG);
    // ENABLE_ASYNC_LOGING();

    TcpServer server;

    std::thread server_worker([s = std::move(server)]() mutable {
        s.serve(8080);
    });

    server_worker.detach();


    // 休息100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 启用客户端
    std::thread cli_loop(&RpcClient::ReadLoop);

    cli_loop.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TestService::TestService_Stub stub;

    int res = stub.add(1, 2);

    LOG_INFO("res: %d", res);

    res = stub.sub(1, 2);

    LOG_INFO("res: %d", res);
    
    return 0;
}
