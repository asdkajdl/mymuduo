#include "FileUtil.h"
#include "Logging.h"


//实现和buffer_貌似没有关系，仅仅是设置了写入时的数据缓冲区
FileUtil::FileUtil(std::string &fileName):fp_(::fopen(fileName.c_str(),"ae")),writtenBytres_(0) {
    ::setbuffer(fp_,buffer_,sizeof(buffer_));
}
FileUtil::~FileUtil() {
    ::fclose(fp_);
}

void FileUtil::append(const char *data, size_t len) {
    size_t written = 0;
    while(written != len){
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if(n!=remain){
            int err = ferror(fp_);//查看上面的write是否出错，0表示没出错，>0出错
            if(err){
                fprintf(fp_,"FileUtil::append() failed %s\n",getErrnoMsg(err));
            }
        }
        written += n;
    }
    // 记录目前为止写入的数据大小，超过限制会滚动日志(滚动日志在LogFile中实现)
    writtenBytres_ += written;
}

void FileUtil::flush() {
    ::fflush(fp_);
}

size_t FileUtil::write(const char *data, size_t len) {
//    fwrite是线程安全的，fwrite_unlocked不是，后者更快
//     第二个参数size，每个数据大小，第三个参数n:数据个数
    return ::fwrite_unlocked(data,1,len,fp_);
}