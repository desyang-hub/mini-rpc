#include "minirpc/core/RpcServer.h"

// ./server 8080
// nacos的注册模块仅允许一个实例运行，同一主机无法运行多个nacos实例
int main(int argc, char const *argv[])
{
    // 启用net模块进行网络连接
    minirpc::RpcServer::GetInstance().Start(8083);

    return 0;
}
