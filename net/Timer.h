#pragma once

#include "../base/nocopyable.h"
#include "../base/Timestamp.h"
#include <functional>

/**
 * 定时器类：一个定时器应该需要知道超时时间，是否重复，如果是重复的定时器就需要知道执行间隔是多少，
 * 还需要知道超时后进行什么样的处理（执行回调函数）
*/
class Timer:nocopyable{
public:
    using TimerCallback = std::function<void()>;
    Timer(TimerCallback cb, Timestamp when, double interval):callback_(std::move(cb)),
    expiration_(when),interval_(interval),repeat_(interval > 0.0){}
    void run(){
        callback_();
    }
    Timestamp expiration() const {return expiration_;}
    bool repeat() const {return repeat_;}
    //重启定时器，其实就是修改一下下次超时的时刻
    void restart(Timestamp now);
private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;//超时间隔，一次性定时器interval应该是0.0
    const bool repeat_;
};