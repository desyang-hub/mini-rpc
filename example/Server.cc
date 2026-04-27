// #include "UserService.h"
#include "minirpc/net/TcpServer.h"

int main(int argc, char const *argv[])
{
    // 启用net模块进行网络连接
    minirpc::TcpServer tcpServer;

    tcpServer.serve(8081);

    return 0;
}
