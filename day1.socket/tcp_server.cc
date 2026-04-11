#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <atomic>
#include <thread>

#include <iostream>

// 用户数量
std::atomic<int> g_thread_count{0};

const int MAX_CLIENT_LIMIT = 3;

void tcp_server(int port = 8080);
void epoll_loop(int sockfd);
void request_handler(int client_fd);



int main() {

    // int port = 8080;
    tcp_server();
    std::cout << "Press Any key quit...";
    std::cin.get();

    return 0;
}


void tcp_server(int port) {
    // 1. 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("create socket error");
        exit(-1);
    }

    // 2. 设置地址复用
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    // 3. bind
    struct sockaddr_in  address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有请求
    address.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&address, sizeof address) < 0) {
        perror("bind error");
        close(sockfd);
        exit(-1);
    }


    std::cout << "Listen port 0.0.0.0:" << port << std::endl;

    // listen
    if (listen(sockfd, 3) < 0) { // 第二个参数表示的是如果等待的数量超过3，直接拒绝
        perror("listen error");
        close(sockfd);
        exit(-1);
    }

    std::thread loop_worker(epoll_loop, sockfd);
    loop_worker.detach();
}


void epoll_loop(int sockfd) {
    int client_fd;
    // 5. 接收请求并服务
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &addr_len);

        // 连接失败
        if (client_fd < 0) {
            continue;
        }

        // 超过数量拒绝服务
        if (g_thread_count >= MAX_CLIENT_LIMIT) {
            close(client_fd);
            continue;
        }

        g_thread_count++;

        // 进入服务
        std::thread client_handler(request_handler, client_fd);
        client_handler.detach(); // 分离线程
    }
}

void request_handler(int client_fd) {
    char buf[1024];
    while (true) {
        // 接收用户消息，并写回
        int byteNum = recv(client_fd, buf, 1024, 0);

        // 断开或者错误
        if (byteNum <= 0) {
            break;
        }

        if (send(client_fd, buf, byteNum, 0) < 0) {
            break;
        }
    }

    close(client_fd);
    g_thread_count--;
}