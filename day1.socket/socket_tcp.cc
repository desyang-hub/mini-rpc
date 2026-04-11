#include <iostream>

#include <sys/socket.h>


void socket_tcp() {
    // 1. socket create 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // socket(domain, type, protocal)
    if (sockfd == -1) {
        perror("socket create error");
        exit(-1);
    }

    // 2. set addr reuse
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    // 3. bind addr


    // 4. listen

    

    // 
}

int main() {


    return 0;
}