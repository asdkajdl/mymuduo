#pragma once

#include "../base/nocopyable.h"
#include "../base/Thread.h"

#include <mutex>
#include <condition_variable>
class EventLoop;
//此类的作用是将EventLoop和Thread联系起来
class EventLoopThread: nocopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),//ThreadInitCallback()??什么道理
    const std::string& name = std::string());
    ~EventLoopThread();
    EventLoop* startLoop();//开启一个新线程
private:
    void threadFunc();
    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};