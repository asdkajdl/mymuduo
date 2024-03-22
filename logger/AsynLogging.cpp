#include "AsynLogging.h"
#include "../base/Timestamp.h"
#include <stdio.h>

AsynLogging::AsynLogging(const std::string &basename, off_t rollSize, int flushInterval)
    :basename_(basename),rollSize_(rollSize),flushInterval_(flushInterval),
    running_(false), thread_(std::bind(&AsynLogging::threadFunc,this),"Logging"),
    mutex_(),cond_(),currentBuffer_(new Buffer),nexBuffer_(new Buffer),
    buffers_()
{
    //currentBuffer_,nexBuffer_ 4M
    currentBuffer_->bzero();
    nexBuffer_->bzero();
    buffers_.reserve(16);//16*4,64M
}
// 前端如果开启异步日志的话，就会调用这个，把日志信息写入currentBuffer_中
void AsynLogging::append(const char *logging, int len) {
//    append由多个线程调用，多个线程将数据写入currentBuffer_中，所以加锁
    std::unique_lock<std::mutex> lock(mutex_);
    if(currentBuffer_->valid() > len){
        currentBuffer_->append(logging,len);
    }else {
        // 当前缓冲区空间不够，将新信息写入备用缓冲区
        // 将写满的currentBuffer_装入vector中，即buffers_中，注意currentBuffer_是独占指针，
        // move后自己就指向了nullptr
        buffers_.push_back(std::move(currentBuffer_));
        if(nexBuffer_){
            currentBuffer_ = std::move(nexBuffer_);
        }else {
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logging,len);
        cond_.notify_one();

    }
}
//线程从AsynLogging创建时就运行,将buffers_中的数据写入文件
void AsynLogging::threadFunc() {
    // output有写入磁盘的接口，异步日志后端的日志信息只会来自buffersToWrite，也就是说不会
    // 来自其他线程，因此不必考虑线程安全，所以output的threadSafe设置为false
    LogFile output(basename_,rollSize_, false);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);//两个后端缓冲区
    newBuffer1->bzero();
    newBuffer2->bzero();

    BuferVector buffersToWrite;
    buffersToWrite.reserve(16);//用户和前端的数组交换
    while(running_){
        {
            //互斥锁保护
            std::unique_lock<std::mutex> lock(mutex_);
            if(buffers_.empty()){
                //等待3秒也会解出阻塞
                cond_.wait_for(lock,std::chrono::seconds(flushInterval_));
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if(!nexBuffer_){
                nexBuffer_ = std::move(newBuffer2);
            }
        }
        for(const auto& buffer: buffersToWrite){
            output.append(buffer->data(),buffer->length());
        }
        //只保留两个缓冲区
        if(buffersToWrite.size() > 2){
            buffersToWrite.resize(2);//留给新缓冲区
        }
        if(!newBuffer1){
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();//std::move后最后的不是没了，是指向nullptr了。所以还要pop_back()
            newBuffer1->reset();
        }
        if(!newBuffer2){
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        //重置bufferToWrite
        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}