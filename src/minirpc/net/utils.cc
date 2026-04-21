#include "minirpc/net/utils.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

namespace minirpc
{

// 用于发起Tcp连接，返回fd
int Dial(int port, const std::string& host) {
    // 先建立连接，拿到fd
    // 1. 创建socket
    int clifd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 目标addr
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(clifd, (sockaddr*)&server_addr, sizeof server_addr) < 0) {
        perror("connect error");
        close(clifd);
        exit(-1);
    }

    return clifd;
}

    
} // namespace minirpc
