#include "minirpc/net/TcpServer.h"

// ./server 8080
// nacos的注册模块仅允许一个实例运行，同一主机无法运行多个nacos实例
int main(int argc, char const *argv[])
{
    // 启用net模块进行网络连接
    minirpc::TcpServer tcpServer;

    int port = 8081;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    try
    {
        tcpServer.serve(port);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    

    return 0;
}
