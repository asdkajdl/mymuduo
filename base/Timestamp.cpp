#include "Timestamp.h"

//获取当前时间戳
Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);//再x86-64平台，gettimeofday已不是系统调用，不会陷入内核
    int64_t seconds = tv.tv_sec;
    //转化为微妙
    return Timestamp(seconds * microSecondsPerSecond + tv.tv_usec);
}
//showMicroseconds是否显示到毫秒
std::string Timestamp::toFormattedString(bool showMicroseconds) const {
    char buf[64] = {0};
    //time_t(long int)
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / microSecondsPerSecond);
    //使用localtime函数将秒数格式化为时间
    tm *tm_time = localtime(&seconds);
    if(showMicroseconds){
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % microSecondsPerSecond);
        snprintf(buf,sizeof(buf),"%4d/%02d/%02d %02d:%02d:%02d.%06d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec,
                 microseconds);
    }else {
        snprintf(buf,sizeof(buf),"%4d/%02d/%02d %02d:%02d:%02d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec);
    }
    return buf;
}