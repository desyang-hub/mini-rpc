#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/timeStamp.h"
#include "minirpc/common/ThreadPool.h"
#include "minirpc/net/Conn.h"
#include "minirpc/net/Channel.h"
#include "minirpc/net/EventLoop.h"

#include <unordered_map>
#include <functional>
#include <atomic>
#include <memory>

namespace minirpc
{

class TcpServer : public nonecopyable
{
private:
    std::function<void(const std::string&)> onMessageCallBack_;
    std::function<void(int fd)> onConnectedCallBack_;

    int sockfd_{-1};
    std::unique_ptr<EventLoop> loop_;
    std::unordered_map<int, std::pair<std::shared_ptr<Conn>, Channel*>> connMap_;
    ThreadPool threadPool_;

    int init(int port);
    void lunch_service_register(int port, const std::string& host="127.0.0.1");
    void loop();
    void removeConn(int fd);
    static void sigHandler(int sig);

    static void ClienHandler(TcpServer* server, std::shared_ptr<Conn> c);

public:
    static std::atomic_bool is_running_;
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
    TcpServer();
    ~TcpServer();

    TcpServer(TcpServer&& other) noexcept
        : nonecopyable()
    {
        this->onMessageCallBack_ = std::move(other.onMessageCallBack_);
        this->onConnectedCallBack_ = std::move(other.onConnectedCallBack_);
        this->sockfd_ = other.sockfd_;
        this->loop_ = std::move(other.loop_);
        this->connMap_ = std::move(other.connMap_);
        other.sockfd_ = -1;
    }
};

} // namespace minirpc
