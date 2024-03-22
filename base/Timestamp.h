#pragma once

#include <iostream>
#include <string>
#include <sys/time.h>

//记录微妙，返回微妙/秒。字符串类型的秒，微妙。时间类型。。。还有失效的时间戳
class Timestamp{
public:
    Timestamp():microSecondsSinceEpoch_(0){}
    explicit Timestamp(int64_t microSecondsSinceEpoch):microSecondsSinceEpoch_(microSecondsSinceEpoch){

    }
    static Timestamp now();
    //用std::string形式返回,格式[millisec].[microsec]
    std::string toString() const;
    //格式, "%4d年%02d月%02d日 星期%d %02d:%02d:%02d.%06d",时分秒.微秒
    std::string toFormattedString(bool showMicroseconds = false) const;
    // 判断时间戳是否有效（只要microSecondsSinceEpoch_>0就是有效）
    bool valid() const {return microSecondsSinceEpoch_ > 0;}
    //返回当前时间戳的微妙
    int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
    time_t secondSinceEpoch() const{
        return static_cast<time_t>(microSecondsSinceEpoch_ / microSecondsPerSecond);
    }
    static Timestamp invalid(){//失效的时间戳，返回一个值为0的Timestamp
        return Timestamp();
    }
    static const int microSecondsPerSecond = 1000 * 1000;
private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp l, Timestamp r){
    return l.microSecondsSinceEpoch() < r.microSecondsSinceEpoch();
}
inline bool operator==(Timestamp l,Timestamp r){
    return l.microSecondsSinceEpoch() == r.microSecondsSinceEpoch();
}
inline Timestamp addTime(Timestamp timestamp , double seconds){
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::microSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}