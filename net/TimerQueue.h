#pragma once
#include "../base/Timestamp.h"
#include "Channel.h"
#include <vector>
#include <set>

class EventLoop;
class Timer;
class TimerQueue{
public:
    using TimerCallback = std::function<void()>;
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();
//    通过insert向TimerList中插入定时器(回调函数，时间，时间间隔)
    void addTimer(TimerCallback cb,Timestamp when,double interval);
private:
    using Entry = std::pair<Timestamp,Timer*>;//我怀疑Timestamp是Timer内的时间值，Timer是对Timestamp的封装，含有时间间隔是否重复
    // set内部是红黑树，删除，查找效率都很高，而且是排序的，set中放pair默认是按照pair的第一个元素进行排序
    // 即这里是按照Timestamp从小到大排序
    using TimerList = std::set<Entry>;
    //在自己所属的loop宏添加定时器
    void addTimerInLoop(Timer* timer);
    //定时器读事件触发函数
    void handleRead();
    //获取到期的定时器
    std::vector<Entry> getExpired(Timestamp now);

    //重置这些到期的定时器(销毁或者重复定时任务)
    void reset(const std::vector<Entry>& expired,Timestamp now);
    bool insert(Timer* timer);//插入定时器的内部方法

    EventLoop* loop_;//所属的EventLoop
    const int timerfd_;//timerfd是linux提供的定时器接口,我懂了 timerfd变为可读时就检查是否有到期的，然后处理
    Channel timerfdChannel_;//封装timerfd文件描述符/...啊？这也有一个fd
    TimerList timers_;//定时器队列
    bool callingExpiredTimers_;//是否正在获取超时器
};