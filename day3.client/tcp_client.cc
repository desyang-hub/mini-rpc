#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <chrono>

void tcp_client(int port=8080, const std::string& host = "0.0.0.0");
void handle(int sockfd);

int main() {

    tcp_client(8080);

    std::cout << "Press Any key quit...";
    std::cin.get();

    return 0;
}

void tcp_client(int port, const std::string& host) {

    // 1. 创建socket
    int clifd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 目标addr
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(clifd, (sockaddr*)&server_addr, sizeof server_addr) < 0) {
        perror("connect error");
        close(clifd);
        exit(-1);
    }

    std::cout << "Connected to " << host << ":" <<port << std::endl;
        std::thread worker(handle, clifd);
        worker.detach();
}


void handle(int sockfd) {
    char buf[1024];

    std::string msg = "hello";

    while (true)
    {
        memset(buf, 0, 1024);
        if (send(sockfd, msg.data(), msg.size(), 0) < 0) {
            perror("send error");
            break;
        }

        int byteNum = recv(sockfd, buf, 1024, 0);
        if (byteNum <= 0) {
            if (byteNum == 0) {
                break;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            break;
        }
        std::cout << "recv: " << buf << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    close(sockfd);
}