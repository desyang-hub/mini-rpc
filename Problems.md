### 注意这个初始化，必须要memset，或者是{}进行默认初始化，否则会发生意想不到的错误，比如connect一直卡住
sockaddr_in server_addr

### RpcClient
socket 需要设置为非阻塞，否则epoll_wait将一直阻塞


### 使用close(sockfd) close(epollfd) 无法触发epoll_wait
epoll_wait 在没有事件时永久阻塞，且无法通过关闭 fd 或修改内存标志可靠地唤醒