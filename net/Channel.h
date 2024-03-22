#pragma once

#include "../base/nocopyable.h"
#include "../base/Timestamp.h"
#include "../logger/Logging.h"

#include<functional>
#include <memory>

class EventLoop;
/**
* Channel封装了fd和其感兴趣的event，如EPOLLIN,EPOLLOUT，还提供了修改fd可读可写等的函数，
* 当fd上有事件发生了，Channel还提供了处理事件的方法
*/
class Channel : nocopyable{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop* loop,int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }
    // 将TcpConnection的共享指针和Channel的成员弱指针绑定tie_，便于在Channel在处理事件时，
    // 防止TcpConnection已经被析构了（即连接已经关闭了）
    void tie(const std::shared_ptr<void> &);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt){revents_ = revt;}//根据revents_来调用不同的回调函数

    //向epoll中注册fd感兴趣的事件，update调用epoll_ctl
    void enableReading(){events_ |= readEvent;update();}
    void disableReading(){events_ &= ~readEvent;update();}
    void enableWriting(){events_ |= writeEvent;update();}
    void disableWriting(){events_ &= ~writeEvent; update();}
    void disableAll(){events_ &= noneEvent; update();}
//    返回当前fd的事件状态
    bool isNonEvent()const {return events_ == noneEvent;}
    bool isWriting()const {return events_ & writeEvent;}
    bool isReading()const {return events_ & readEvent;}

    /**
     * 返回fd在EPoller中的状态
     * for EPoller
     * const int kNew = -1;     // fd还未被Epoller监视
     * const int kAdded = 1;    // fd正被Epoller监视中
     * const int kDeleted = 2;  // fd被从Epoller中移除
     */

    int index() const {return index_;}
    void set_index(int index){index_= index;}

    //返回Channle自己所属的loop
    EventLoop* ownerLoop() {return loop_;}

    // 从EPoller中移除自己，也就是让EPoller停止关注自己感兴趣的事件，
    // 这个移除不是销毁Channel，而是只改变channel的状态，即index_,
    // Channel的生命周期和TcpConnection一样长，因为Channel是TcpConnection的成员，
    // TcpConnection析构的时候才会释放其成员Channel
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    static const int noneEvent;
    static const int readEvent;
    static const int writeEvent;
    EventLoop* loop_;
    const int fd_;
    int events_;// 注册fd感兴趣的事件
    int revents_;// poller返回的具体发生的事件
    int index_;//在EPoller上注册的状态，状态有New added deleted

    std::weak_ptr<void> tie_;//弱指针指向Tcpconnetion必要时升级为shared_ptr多一份引用计数，避免用户误删
    bool tied_;//标志此Channle是否调用过tie方法
//    四个回调函数调用的都是TcpConnect的函数
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};