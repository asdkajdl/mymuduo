#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const EventLoopThread::ThreadInitCallback &cb, const std::string &name)
    :loop_(nullptr),exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc,this),name),
    mutex_(),cond_(),callback_(cb)
{
//    thread线程调用了EventLoopThread的回调函数
}
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop() {
    // 启动一个新线程。在构造函数中，thread_已经绑定了回调函数threadFunc
    // 此时，子线程开始执行threadFunc函数
    thread_.start();
    EventLoop* loop = nullptr;
    {
        // 等待新线程执行创建EventLoop完毕，所以使用cond_.wait
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr){
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}
void EventLoopThread::threadFunc() {
    EventLoop eventLoop;
    // 用户自定义的线程初始化完成后要执行的函数
    if(callback_){
        callback_(&eventLoop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);

        loop_ = &eventLoop;//子线程调用的threadFunc，然后修改了父线程中的loop_,将子线程创建的
//        临时变量eventLoop传递了出去
        cond_.notify_one();
    }
    // 执行EventLoop的loop() 开启了底层的EPoller的poll()
    // 这个是subLoop
    eventLoop.loop();
    // loop是一个事件循环，如果往下执行说明停止了事件循环，需要关闭eventLoop
    // 此处是获取互斥锁再置loop_为空
    std::unique_lock<std::mutex> lock(mutex_);//为什么要获取一个锁?不是就一个Thread吗
    loop_ = nullptr;
}