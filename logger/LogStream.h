#pragma once
#include "FixedBuffer.h"

//GeneralTemplate:用于LogStream<<时间，sourceFIle等
class GeneralTemplate:nocopyable{
public:
    GeneralTemplate():data_(nullptr),len_(0){}
    explicit GeneralTemplate(const char* data, int len):data_(data),len_(len){}
    const char* data_;
    int len_;
};

/**
 * LogStream： 实现类似cout的效果，便于输出日志信息，即：LogStream << A << B << ...
 * 要实现类似cout的效果，就需要重载“<<”
*/
class LogStream:nocopyable{
public:
    using SmallBuffer = FixedBuffer<samllBuffer>;//4k
    void append(const char* data, int len){
        buffer_.append(data,len);
    }
    const SmallBuffer& buffer() const {return buffer_;}
    void resetBuffer() {buffer_.reset();}
    // 重载运算符"<<"，以便可以像cout那样 << 任意类型
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);
    LogStream& operator<<(float v);
    LogStream& operator<<(double v);
    LogStream& operator<<(char c);
    LogStream& operator<<(const void* p);
    LogStream& operator<<(const char* str);
    LogStream& operator<<(const unsigned char* str);
    LogStream& operator<<(const std::string& str);
    LogStream& operator<<(const SmallBuffer& buf);
    LogStream& operator<<(const GeneralTemplate& g);// 相当于(const char*, int)的重载
private:
    // 数字（整数或者浮点数）最终是要被写入buffer_中的，因此要把数字转化为字符串，
    // 向buffer_中写入数字对应的字符串时，buffer_需要通过maxNumberSize来判断
    // 自己是否还能够写的下
    static const int maxNumberSize = 48;
    SmallBuffer buffer_;

    template<typename T>
    void formatInteger(T);
};
