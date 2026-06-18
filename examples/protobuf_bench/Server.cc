#include "minirpc/core/RpcServer.h"
#include "minirpc/common/Config.h"

int main(int argc, char const *argv[])
{
    auto cfg = minirpc::loadConfig();

    minirpc::RpcServer::GetInstance()
        .Start(cfg.port, "RpcServer", cfg.registry_address.c_str());

    return 0;
}
