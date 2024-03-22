#include "Timer.h"

void Timer::restart(Timestamp now) {
    if(repeat_){
        //设置新的到期时间
        expiration_ = addTime(now,interval_);//now + inverval_,里面是微妙
    }else {
        expiration_ = Timestamp::invalid();//失效的Timestamp,值为0
    }
}