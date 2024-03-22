#include "CurrentThread.h"

namespace currentThread{
    __thread pid_t t_cachedTid = 0;
    void CacheTid(){
        if(t_cachedTid == 0){
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));//syscall(SYS_gettid)系统调用，获取线程的id
        }
    }
}