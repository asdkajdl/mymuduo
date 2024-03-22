#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &name)
    :baseLoop_(baseLoop),name_(name),started_(false),numThreads_(0),next_(0)
{

}
EventLoopThreadPool::~EventLoopThreadPool() {
    // Don't delete loop, it's stack variable.
//    EventLoop是在栈中创建出来的，脱离变量区域自动析构，上层的获得的是指向它的指针
}
void EventLoopThreadPool::start(const EventLoopThreadPool::ThreadInitCallback &cb) {
    started_ = true;//单个线程不需要mutex
    //循环创建线程
    for(int i = 0; i < numThreads_; i++){
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf),"%s%d",name_.c_str(),i);
        EventLoopThread* t = new EventLoopThread(cb,buf);
        // 加入此EventLoopThread入容器
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // 底层创建线程 绑定一个新的EventLoop 并返回该loop的地址
        // 此时已经开始执行新线程了
        loops_.push_back(t->startLoop());//startLoop会获取栈中的loop指针并返回
    }
    //整个服务端只有一个线程运行baseLoop
    if(numThreads_ == 0){
        cb(baseLoop_);
    }
}
//如果工作在多线程中，baseLoop_会以轮询的方式选择一个subloop，以便后续将连接分发给这个subloop
EventLoop *EventLoopThreadPool::getNextLoop() {
    // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor
    // 那么轮询只有一个线程 getNextLoop()每次都返回当前的baseLoop_
    EventLoop *loop = baseLoop_;
    if(!loops_.empty()){
        if(next_ >= loops_.size()){
            next_ = 0;
        }
        loop = loops_[next_];
        next_++;
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    if(loops_.empty()){
        return std::vector<EventLoop*>(1,baseLoop_);
    }else {
        return loops_;
    }
}