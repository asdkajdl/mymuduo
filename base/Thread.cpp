#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int32_t Thread::numCreated_(0);
Thread::Thread(ThreadFunc func, const std::string &name):started_(false)
,joined_(false),tid_(0),func_(std::move(func)),name_(name)
{
    setDefaultName();
}
Thread::~Thread() {
    if(started_ && !joined_){
        thread_->detach();
    }
}

void Thread::start() {
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);//pshared:0信号量线程间共享，1则是进程间共享，value表示信号量初始值

    thread_ = std::make_shared<std::thread>([&](){
        tid_ = currentThread::tid();
        sem_post(&sem);//信号量+1
        func_();
    });
    /**
     * 这里必须等待获取上面新创建的线程的tid
     * 未获取到信息则不会执行sem_post，所以会被阻塞住
     * 如果不使用信号量操作，则别的线程访问tid时候，可能上面的线程还没有获取到tid
     */
    sem_wait(&sem);
}
void Thread::join() {
    joined_ = true;
    //等待线程执行完毕
    thread_->join();
}
void Thread::setDefaultName() {
    int num = ++numCreated_;
    if(name_.empty()){
        char buf[32] = {0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        name_ = buf;

    }
}
