#pragma once

#include "../base/nocopyable.h"
#include <assert.h>
#include <string>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

/**
* 自定义Buffer工具类,将前端信息吸入Buffer，然后后端从Bufer取出输出到文件
*/

const int samllBuffer = 4096;
const int largBuffer = 4096 * 1024;
template<int SIZE>//Buffer size
class FixedBuffer: nocopyable{
public:
    FixedBuffer():cur_(data_){}
    void append(const char *buf,size_t len){
        if(static_cast<size_t>(valid()) > len){
            ::memcpy(cur_,buf,len);
            cur_ += len;
        }
    }
    int valid(){
        return static_cast<int>(end() - cur_);
    }
    const char* data() const{return data_;}
    int length() const{return static_cast<int>(cur_ - data_);}
    char* current(){return cur_;}

    void add(size_t len){cur_ += len;}
    void reset(){cur_ = data_;}
    void bzero(){ ::bzero(data_,sizeof(data_)); }
    std::string toString() const {return std::string(data_,length());}
private:
    const char* end() const{ return data_ + sizeof(data_);}
    char data_[SIZE];//不是指针
    char *cur_;//记录data已经写到什么位置
    /*------------------------------------*/
    /*|xxxxxxxxxxxxx|<---   avail() ---->|*/
    /*------------------------------------*/
    /*data_        cur_              end()*/
};
