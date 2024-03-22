#pragma once

#include <unistd.h>
#include <syscall.h>

namespace currentThread{
    extern __thread pid_t t_cachedTid;//extern表明t_cachedTid再别处定义过，这里要用。__thread保证各个线程中t_cachedTid唯一。保存tid缓冲，避免多次系统调用
    void CacheTid();
    inline int tid(){
        if(__builtin_expect(t_cachedTid == 0, 0)){//__builtin_expect是gcc自带的，表明__builtin_expect(EXP,P),告诉gpu事件EXP的执行结果大概率为P，从而优化cpu缓存，加快执行效率
//            本质是if(t_cachedTid == 0)//其实就是加快跳过cacheTid
            CacheTid();
        }
        return t_cachedTid;
    }

}