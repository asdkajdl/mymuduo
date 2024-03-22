/**
 * LogFile对写入File做了封装，构造文件名，什么实现rollFile，是真正写入文件的
 * log则是对前端的封装，FixedBuffer存数据，LogStream重载<<方法，Logging再LogStream基础上封装开头和结尾以及LOG_leveL定义。最终数据都在FixedBuffer中
 * 名字log无File全都是封装，属于前端的东西，有File的才是后端，AsynLogging要把两个结合起来
 *
 */
#pragma once

#include "../base/nocopyable.h"
#include "../base/Thread.h"
#include "FixedBuffer.h"
#include "LogStream.h"
#include "LogFile.h"

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

class AsynLogging : nocopyable{
public:
    AsynLogging(const std::string& basename,off_t rollSize,int flushInterval = 3);
    ~AsynLogging(){
        if(running_){
            stop();
        }
    }
    //前端调用写入日志
    void append(const char* logging,int len);
    void start(){
        running_ = true;
        thread_.start();
    }
    void stop(){
        running_ = false;
        cond_.notify_one();//只有一个线程在写
        thread_.join();
    }
private:
    using Buffer = FixedBuffer<largBuffer>;//4M
//    BufferVector存储独占指针的目的是：从BufferVector中弹出元素（Buffer）后会自动析构
    using BuferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BuferVector::value_type;
    void threadFunc();//??

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;//日志文件名的公共部分
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BuferVector buffers_;
    BufferPtr currentBuffer_;
    BufferPtr nexBuffer_;
};