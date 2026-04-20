#pragma once

#include "minirpc/common/nonecopyable.h"
#include "minirpc/common/timeStamp.h"

#include <memory>
#include <functional>


namespace minirpc
{

class EventLoop;

// one loop per thread.
// 封装感兴趣的fd和event, epollIN epollOUT时间
/// 还有poller返回的事件
class Channel : public nonecopyable
{
private:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(const TimeStamp&)>;
public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(const TimeStamp& timeStamp);

    // start ================ 设置回调事件 ================
    void setReadEventCallback(ReadEventCallback cb) {
        readEventCallback_ = std::move(cb);
    }

    void setWriteEventCallback(EventCallback cb) {
        writeEventCallback_ = std::move(cb);
    }

    void setCloseEventCallback(EventCallback cb) {
        closeEventCallback_ = std::move(cb);
    }

    void setErrorEventCallback(EventCallback cb) {
        errorEventCallback_ = std::move(cb);
    }

    // end ================ 设置回调事件 ================

    // 防止channel被手动析构后，还在回调函数
    void tie(const std::shared_ptr<void>& bind) {
        tie_ = bind;
        tied_ = true;
    }

    // sock fd
    int fd() const {
        return fd_;
    }

    // interesting envents
    int events() const {
        return events_;
    }

    // set real trigger event to renents_
    void setREvents(int revents) {
        revents_ = revents;
    }

    void update();

    // seting reading event interesting
    void enableReading() {
        events_ |= kReadEvent;
        update();
    }

    // seting reading event not interesting
    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }

    // seting writing event interesting
    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }

    // seting writing event not interesting
    void disableWriting() {
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll() {
        events_ = kNoneEvent;
        update();
    }

    bool isNoneEvent() const {
        return events_ == kNoneEvent;
    }

    bool isReading() const {
        return events_ & kReadEvent;
    }

    bool isWriting() const {
        return events_ & kWriteEvent;
    }

    int index() const {
        return index_;
    }

    void set_index(int index) {
        index_ = index;
    }

    EventLoop* ownerLoop() const {
        return loop_;
    }

    // remove channel from EventLoop
    void remove();

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    int fd_;
    int events_{0}; // 感兴趣事件
    int revents_{0}; // 具体发生的事件
    int index_{-1};

    // 使用weak_ptr监控别的对象，避免对象已经释放了却依然被调用
    std::weak_ptr<void> tie_;

    // tie_ bind state
    bool tied_{false}; 

    // 因为channel能够从epoll获取发生的事件revents, 从而调用对应的事件回调函数
    ReadEventCallback readEventCallback_;
    EventCallback writeEventCallback_;
    EventCallback closeEventCallback_;
    EventCallback errorEventCallback_;

    void handleEventWithGuard(const TimeStamp& timeStamp);
};

    
} // namespace minirpc
