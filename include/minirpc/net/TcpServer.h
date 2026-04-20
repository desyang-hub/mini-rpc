#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/timeStamp.h"
#include "minirpc/net/Conn.h"


#include <iostream>
#include <unordered_map>
#include <functional>

namespace minirpc
{

void set_nonblocking(int fd);

class TcpServer : public nonecopyable
{
private:
    std::function<void(const std::string&)> onMessageCallBack_; // 消息回调
    std::function<void(int fd)> onConnectedCallBack_; // 连接回调

    int sockfd_;
    int epollfd_;
    std::unordered_map<int, Conn*> connMap_;
    

    int init(int port);
    void loop();
    void removeConn(Conn* c);

    static void ClienHandler(TcpServer* server, Conn* c);

public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(const TimeStamp&)>;

    void setOnMessageCallBack(std::function<void(const std::string&)> cb) {
        onMessageCallBack_ = std::move(cb);
    }

    void setOnConnectedCallBack(std::function<void(int fd)> cb) {
        onConnectedCallBack_ = std::move(cb);
    }

    void serve(int port=8080);

public:
    TcpServer(/* args */);
    ~TcpServer();

    // 移动构造函数
    TcpServer(TcpServer&& other) noexcept
        : nonecopyable() // 1. 关键！显式调用基类的默认构造函数
    {
        // 2. 手动移动所有成员变量
        this->onMessageCallBack_ = std::move(other.onMessageCallBack_);
        this->onConnectedCallBack_ = std::move(other.onConnectedCallBack_);
        this->sockfd_ = other.sockfd_;
        this->epollfd_ = other.epollfd_;
        this->connMap_ = std::move(other.connMap_);

        // 3. 将“被移动”的对象资源置为无效状态，这是一个好习惯
        other.sockfd_ = -1;
        other.epollfd_ = -1;
    }

    // TcpServer& operator=(TcpServer&&) = default;
};


} // namespace minirpc
