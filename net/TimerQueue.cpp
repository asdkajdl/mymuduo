#include "TimerQueue.h"
#include "Timer.h"
#include "EventLoop.h"
#include "../logger/Logging.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

int createTimerfd(){
    // 创建timerfd
    // CLOCK_MONOTONIC表示绝对时间（最近一次重启到现在的时间，会因为系统时间的更改而变化）
    // CLOCK_REALTIME表示从1970.1.1到目前时间（更该系统时间对其没影响）
    int timerFd = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
    LOG_DEBUG<<"create a timerfd, fd = "<<timerFd;
    if(timerFd < 0)LOG_ERROR<<"failed in timerfd_create";
    return timerFd;
}
//重置timerfd的超时时刻，重置后如果超时时刻不为0，则内核会启动定时器，否则内核会停止定时器
void resetTimerfd(int timerfd_,Timestamp expiration){//真正的设置超时时间，内核能通知的那种，其他的都是封装
//    还剩多长时间超时，算出来告诉内核，内核定时，通过timerfd_settime，格式为itimerspec
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue,'\0',sizeof(newValue));
    memset(&oldValue,'\0',sizeof(newValue));
    // 计算多久后计时器超时（超时时刻 - 现在时刻）
    int64_t microSecondDif = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if(microSecondDif < 100){
        microSecondDif = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microSecondDif / Timestamp::microSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(microSecondDif % Timestamp::microSecondsPerSecond * 1000);
    newValue.it_value = ts;
//    调用timerfd_settime会在内核启动定时器
    if(::timerfd_settime(timerfd_,0,&newValue,&oldValue)){
        LOG_ERROR<<"timerfd_settime failed";
    }
}
void readTimerfd(int timerfd){//fd可读，证明时间到了，什么都不干，相当于默认的read处理
    uint64_t howmany;
    ssize_t n = ::read(timerfd,&howmany, sizeof(howmany));
    if(n != sizeof(howmany)){
        LOG_ERROR<<"TimerQueue::handleRead() reads " << n <<"bytes instead of 8";
    }
}
TimerQueue::TimerQueue(EventLoop *loop):
    loop_(loop),timerfd_(createTimerfd()), timerfdChannel_(loop_,timerfd_),timers_()
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead,this));
    timerfdChannel_.enableReading();
}
TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    //删除所有定时器
    for(const Entry& timer: timers_){
        delete timer.second;
    }
}
void TimerQueue::addTimer(TimerQueue::TimerCallback cb, Timestamp when, double interval) {
    Timer* timer = new Timer(std::move(cb),when,interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop,this,timer));
}
void TimerQueue::addTimerInLoop(Timer *timer) {
    // 将timer添加到TimerList时，判断其超时时刻是否是最早的
    bool eraliestChanged = insert(timer);
    // 如果新添加的timer的超时时刻确实是最早的，就需要重置timerfd_超时时刻
    if(eraliestChanged){
        resetTimerfd(timerfd_,timer->expiration());
    }
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;//存储到期的定时器
    Entry  sentry(now,reinterpret_cast<Timer*>(UINTPTR_MAX));
    // lower_bound返回第一个大于等于sentry的迭代器，
    // 这里的意思是在TimerList中找到第一个没有超时的迭代器
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(),end,back_inserter(expired));//back_inserter将timers中的添加到expired的末尾
    timers_.erase(timers_.begin(),end);
    return expired;
}

void TimerQueue::handleRead() {//从timers里面拿出所有超时的，然后处理，可能要重置然后插入
    Timestamp now = Timestamp::now();
    readTimerfd(timerfd_);//什么都没发生，就是读了一下
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    for(const Entry& it : expired){
        it.second->run();//每一个Timer中都存在一个定时任务，
    }
    callingExpiredTimers_ = false;
    reset(expired,now);//重置这些定时器，如果是重复的，那么就再插入set中，同时要更新timerfd的时间
}
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
    Timestamp nextExpire;
    for(const Entry& it : expired){
        if(it.second->repeat()){
            Timer* timer = it.second;
            timer->restart(now);//感觉逻辑有点问题，传进入now，不应该是timer里面的timestamp吗
            insert(timer);
        }else {
            delete it.second;
        }
    }
//    如果重新插入的定时器，需要重置timerFd的超时时刻
    if(!timers_.empty()){
        //timerfd下一次可读时刻就是TimerList中第一个Timer的超时时刻
        nextExpire = timers_.begin()->second->expiration();
    }
    if(nextExpire.valid()){
        resetTimerfd(timerfd_,nextExpire);
    }
}
bool TimerQueue::insert(Timer *timer) {
    bool earliestChanged = false;
    Timestamp timestamp = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || timestamp < it->first){//空或者timestamp最早
        // 说明最早的超时的定时器已经被替换了
        earliestChanged = true;
    }
    timers_.insert({timestamp,timer});
    return earliestChanged;
}