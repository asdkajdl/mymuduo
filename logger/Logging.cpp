#include "Logging.h"
#include "../base/CurrentThread.h"

namespace ThreadInfo{
    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;
};
const char* getErrnoMsg(int savedErrno){
//    strerror_r 成功返回0  失败返回 -1 并设置errno
    return strerror_r(savedErrno,ThreadInfo::t_errnobuf,sizeof(ThreadInfo::t_errnobuf));
}

//LEVEL_COUNT用来计数
const char* getLevelName[Logger::LogLevel::LEVEL_COUNT]
        {
                "TRACE ",
                "DEBUG ",
                "INFO  ",
                "WARN  ",
                "ERROR ",
                "FATAL ",
        };

Logger::LogLevel initLogLevel(){
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

//数据写入stdout
static void defaultOutput(const char* data, int len){
    fwrite(data,sizeof(char),len,stdout);//fwrite(data, len, sizeof(char), stdout);原本是这个，应该写错了
}
static void defaultFlush(){
    fflush(stdout);
}

Logger::OutputFunc f_output = defaultOutput;
Logger::FlushFunc f_flush = defaultFlush;

Logger::Impl::Impl(Logger::Impl::LogLevel level, int savedErrno, const char *filename, int line)
:time_(Timestamp::now()), stream_(),level_(level),line_(line), basename_(filename)
{
    formatTime();
    stream_ << GeneralTemplate(getLevelName[level],6);
    if(savedErrno != 0){
        stream_ << getErrnoMsg(savedErrno) << "(errno="<<savedErrno<<")";
    }
}
void Logger::Impl::formatTime() {
    Timestamp now = Timestamp::now();
    time_t  seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::microSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::microSecondsPerSecond);
    struct tm* tm_time = localtime(&seconds);
    snprintf(ThreadInfo::t_time,sizeof(ThreadInfo::t_time),"%4d/%02d/%02d %02d:%02d%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    ThreadInfo::t_lastSecond = seconds;
    // muduo使用Fmt格式化整数，这里我们直接写入buf
    char buf[32] = {0};
    snprintf(buf,sizeof(buf),"%06d",microseconds);
    stream_<< GeneralTemplate(ThreadInfo::t_time,17)<< GeneralTemplate(buf,7);
}
//日志最后的输出
void Logger::Impl::finish() {
    stream_<<"-"<<GeneralTemplate(basename_.data_,basename_.size_)<<":"<<line_<<'\n';
}
//level默认为INFO等级
Logger::Logger(const char *filename, int line): impl_(INFO,0,filename,line) {

}
Logger::Logger(const char *filename, int line, Logger::LogLevel level): impl_(level,0,filename,line) {

}
//可以打印调用函数
Logger::Logger(const char *filename, int line, Logger::LogLevel level, const char *func) : impl_(level,0,filename,line){
    impl_.stream_<<func<<' ';
}
Logger::~Logger() {
    impl_.finish();
    //获取buffer(stream_.buffer_)
    const LogStream::SmallBuffer& buf=stream().buffer();
    f_output(buf.data(),buf.length());//length是data_到cur_的长度，不是整个data的长度
    if(impl_.level_ == FATAL){
        f_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}
void Logger::setOutput(Logger::OutputFunc outputFunc) {
    f_output = outputFunc;
}
void Logger::setFlush(Logger::FlushFunc flushFunc) {
    f_flush = flushFunc;
}
