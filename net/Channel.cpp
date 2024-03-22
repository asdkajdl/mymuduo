#include "Channel.h"
#include "EventLoop.h"

#include <sys/epoll.h>

const int Channel::noneEvent = 0;
const int Channel::readEvent = EPOLLIN | EPOLLPRI;
const int Channel::writeEvent = EPOLLOUT;
Channel::Channel(EventLoop *loop, int fd)
    :loop_(loop),fd_(fd),
    events_(0),revents_(0),index_(-1),tied_(false)
{

}
Channel::~Channel() {
    if(loop_->isInLoopThread()){
        assert(!loop_->hasChannel(this));
    }
}
void Channel::tie(const std::shared_ptr<void> &obj) {
    // 让tie_(weak_ptr)指向obj，后边处理fd上发生的事件时，就可以用tie_判断TcpConnection是否还活着
    tie_ = obj;
    tied_ = true;
}

void Channel::update() {
//    通过该channel所属的EventLoop，调用EPoller对应的方法，注册fd的events事件
    loop_->updateChannel(this);
}
void Channel::remove() {
    loop_->removeChannel(this);
}
void Channel::handleEvent(Timestamp receiveTime) {
    if(tied_){
        // 变成shared_ptr增加引用计数，防止误删
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
        //guard为空说明Channel的TcpConnection对象已经不在了
    }else {
        handleEventWithGuard(receiveTime);
    }
}
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    // 对方关闭连接会触发EPOLLHUP，此时需要关闭连接
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        if(closeCallback_)closeCallback_();
    }

    //错误事件
    if(revents_ & EPOLLERR){
        if(errorCallback_)errorCallback_();
    }
    // EPOLLIN表示普通数据和优先数据可读，EPOLLPRI表示高优先数据可读，EPOLLRDHUP表示TCP连接对方关闭或者对方关闭写端
    // (个人感觉只需要EPOLLIN就行），则处理可读事件
    if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)){
        if(readCallback_)readCallback_(receiveTime);
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_)writeCallback_();
    }
}