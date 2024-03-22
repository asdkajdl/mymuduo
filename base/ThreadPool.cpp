#include "ThreadPool.h"
#include "../logger/Logging.h"

ThreadPool::ThreadPool(const std::string &name):mutex_(),cond_(),name_(name), running_(false) {
}
ThreadPool::~ThreadPool() {
    if(running_){
        stop();
    }
}
void ThreadPool::start(int numThreads) {
    running_ = true;
    threads_.reserve(numThreads);//resize会自动构造不存在的元素，reserve只是留出空间，而不是早元素
    for(int i = 0; i < numThreads; ++i){
        char id[32];
        snprintf(id,sizeof(id),"%d",i+1);
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread,this),name_+id));
        threads_[i]->start();
    }
    // 如果不创建线程则直接在本线程中执行回调
    if(numThreads == 0 && threadInitCallback_){
        threadInitCallback_();
    }
}
void ThreadPool::stop() {
    running_ = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.notify_all();
    }
    //等待所有子线程退出
    for(auto& thr:threads_){
        thr->join();
    }
}
size_t ThreadPool::queueSize() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
}
void ThreadPool::run(ThreadPool::Task f) {
    if(threads_.empty()){
        f();
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push_back(std::move(f));
    cond_.notify_one();
}
void ThreadPool::runInThread() {//大量的这种子程序运行父程序的函数有什么用？？答：不用传参数，，子程序能直接调用cond_和queue
    try{
        if(threadInitCallback_){
            threadInitCallback_();
        }
        while(running_){
            Task task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock,[this](){
                   return !queue_.empty() && running_;
                });
                if(!queue_.empty()){
                    task = queue_.front();
                    queue_.pop_front();
                }
            }
            if(task){
                task();
            }
        }
    }catch (...){
        LOG_WARN << "runInThread throw exception";
    }
}