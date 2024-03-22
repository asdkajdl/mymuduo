#pragma once

#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <atomic>

class Thread{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc,const std::string &name = std::string());
    ~Thread();
    void start();
    void join();
    bool started() const{ return started_;}
    pid_t tid() const { return tid_;}
    const std::string& name() const {return name_;}
    static int numCreated(){return numCreated_;}
private:
    void setDefaultName();
    std::string name_;
    pid_t tid_;//线程id
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    static std::atomic_int32_t numCreated_;//线程索引
    ThreadFunc func_;
};
