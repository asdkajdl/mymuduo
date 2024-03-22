#pragma once

#include "FileUtil.h"
#include <mutex>
#include <memory>

//LogFile类用于向文件中写入日志信息，具有在合适时机创建日志文件以及把数据写入文件的功能

class LogFile{
public:
    LogFile(const std::string &basename, off_t rollSize,bool threadSafe = true, int flushInterval = 3, int checkEveryN = 1024);
    ~LogFile() = default;

    //向file_缓冲区古buffer_中继续添加日志信息
    void append(const char* logline, int len);
    void flush();
    bool rollFile();
private:
    static std::string getLogFileName(const std::string& basename, time_t* now);
    void append_unlocked(const char* logline, int len);

    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval_;

    const int checkEveryN_;
    int count_;//记录往buffer_添加数据的次数,超过checkEveryN_就fflush

    std::unique_ptr<std::mutex> mutex_;//是否初始化和同步异步有关
    time_t startOfPeriod_;//记录最后一个日志文件是哪一天创建的（单位是秒）
    time_t lastRoll_;//最后一次滚动日志的时间（s）
    time_t lastFlush_;//最后一次刷新的时间
    std::unique_ptr<FileUtil> file_;
    const static int RollPerSeconds_ = 60 * 60 *24;//1day

};