#include "minirpc/core/RpcServer.h"
#include "minirpc/common/Config.h"

#include <iostream>

#include "minirpc/protocol/Decoder.h"
#include "minirpc/protocol/Encoder.h"

using namespace minirpc;

int main(int argc, char const *argv[])
{
    auto cfg = minirpc::loadConfig();
    RpcServer::GetInstance().Start(cfg.port, "RpcServer", cfg.registry_address.c_str());
    return 0;

    // std::string srvName = "aaa";
    // std::string body = "AAA";

    // Bytes bytes = Encoder::Encode(srvName, body.c_str(), body.size());

    // std::string name;
    // std::string b;
    // Decoder::Decode(bytes.data(), name, b);

    // std::cout << name << " " << b << std::endl;
}
