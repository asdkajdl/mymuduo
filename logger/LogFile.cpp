#include "LogFile.h"

LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe, int flushInterval, int checkEveryN)
    :basename_(basename),rollSize_(rollSize),flushInterval_(flushInterval),checkEveryN_(checkEveryN),
    count_(0),mutex_(threadSafe?new std::mutex: nullptr),startOfPeriod_(0),lastRoll_(0),lastFlush_(0)
{

}

void LogFile::flush() {
    if(mutex_){
        std::unique_lock<std::mutex> lock(*mutex_);
        file_->flush();
    }else {
        file_->flush();
    }
}

std::string LogFile::getLogFileName(const std::string &basename, time_t *now) {//产生日志文件名字
    std::string fileName;
    fileName.reserve(basename.size() + 64);
    fileName += basename;
    char buf[32];
    *now = time(nullptr);
    struct tm tim;
    localtime_r(now,&tim);//线程安全，localtime返回静态数据，多线程下返回值可能被覆盖
    strftime(buf,sizeof buf,".%Y%m%d-%H%M-%S",&tim);
    fileName += buf;
    fileName += ".log";
    return fileName;
}

void LogFile::append(const char *logline, int len) {
    if(mutex_){
        std::unique_lock<std::mutex> lock(*mutex_);
        append_unlocked(logline,len);
    }else {
        append_unlocked(logline,len);
    }
}

//已写入字节超过阈值新建文件，写入次数>阈值刷新，如果今天还有新建文件就新建
void LogFile::append_unlocked(const char *logline, int len) {
    file_->append(logline,len);
    if(file_->writtenBytes() > rollSize_){//写入byte超了
        rollFile();
    }else {
        count_++;
        if(count_ >= checkEveryN_){
            count_ = 0;
            time_t t = time(nullptr);
            time_t thisPeriod = t / RollPerSeconds_ * RollPerSeconds_;//到今天0点的秒数
            if(thisPeriod != startOfPeriod_){//行数超了而且到了新的一天
                rollFile();
            }else if(t - lastFlush_ > flushInterval_){
                lastFlush_ = t;
                file_->flush();
            }
        }
    }
}

bool LogFile::rollFile() {
    time_t now = 0;

    std::string fileName = getLogFileName(basename_,&now);
    time_t thisPeriod = now / RollPerSeconds_ * RollPerSeconds_;
    if(lastRoll_ != now){//如果刚rollFile就不做了
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = thisPeriod;//过0点，字节超限rollFile了,防止后面再rollFile
        file_.reset(new FileUtil(fileName));
        return true;
    }
    return false;
}
