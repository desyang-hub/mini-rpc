#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <iostream>

#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>

// 关键， 设置非阻塞IO
// ✅ 正确的函数名和实现
void set_nonblocking(int fd);

// 用户数量
std::atomic<int> g_thread_count{0};
const int MAX_CLIENT_LIMIT = 8;


void tcp_server(int port = 8080);
void epoll_loop(int sockfd);
void request_handler(int client_fd, int epoll_fd);

// 添加使用单线程实现IO多路监听


int main() {

    // int port = 8080;
    tcp_server();
    std::cout << "Press Any key quit...";
    std::cin.get();

    return 0;
}


void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get error");
        return;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
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


    // 设置非阻塞
    set_nonblocking(sockfd);

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

    // 1. 创建epoll
    int epollfd = epoll_create(1024);
    if (epollfd < 0) {
        perror("epoll create error");
        close(sockfd);
        exit(-1);
    }

    // 2. 注册fd监听到epoll
    epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN; // 默认LT水平触发，只要没有完全读完，消息就会一直发送

    std::cout << "Epoll LT Mode epfd=" << epollfd << std::endl;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll add sockfd error");
        close(sockfd);
        close(epollfd);
        exit(-1);
    }

    // 3. 进行事件监听
    // epoll_event events[1024];
    std::vector<epoll_event> events(1024);
    

    while (true) {
        // 阻塞等待，返回触发的事件个数
        int n = epoll_wait(epollfd, events.data(), events.size(), -1); // -1 表示无限等待

        if (n == -1) {
            if (errno == EINTR) continue; // 被信号终端
            perror("epoll wait error");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == sockfd) { // 这个是sockfd的用户请求事件
                sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);

                int client_fd = accept(fd, (sockaddr*)&client_addr, &len);

                if (client_fd < 0) {
                    // accept失败
                    continue;
                }

                set_nonblocking(client_fd); // 设置非阻塞

                // 将事件注册到epoll
                epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;

                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                    close(client_fd);
                    continue;
                }
            }
            else {
                // 这里是一个用户client_fd 事件

                // 由于是LT触发，这里检索一个条件，如果线程超过限制，则暂时不处理
                if (g_thread_count >= MAX_CLIENT_LIMIT) {
                    continue;
                }

                g_thread_count++;

                // 最好使用线程池来进行调度，否则开销巨大
                std::thread request_woker(request_handler, fd, epollfd);
                request_woker.detach();
            }
        }
    }

    close(epollfd);
    close(sockfd);
}

void request_handler(int client_fd, int epoll_fd) {
    char buf[1024];

    // LT
    // 接收用户消息，并写回
    int byteNum = recv(client_fd, buf, 1024, 0);

    // 断开或者错误
    if (byteNum <= 0) {
        if (byteNum == 0) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
            close(client_fd);
        }
        else if (errno == EAGAIN || EWOULDBLOCK) {
            // 暂时没有消息
        }
        else {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
            // 异常
            close(client_fd);
        }
    }
    else if (send(client_fd, buf, byteNum, 0) < 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
    }
    g_thread_count--;
}