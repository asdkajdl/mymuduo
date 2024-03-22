#include "LogStream.h"
#include <algorithm>

// 用于把数字转化位对应的字符
static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                              '1', '2', '3', '4', '5', '6', '7', '8', '9'};
// 用于打印16进制的指针类型
const char digitsHex[] = "0123456789ABCDEF";

size_t convertHex(char buf[], uintptr_t value){
    uintptr_t i = value;
    char *p = buf;
    do{
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    }while(i!=0);
    *p = '\0';
    std::reverse(buf,p);
    return p - buf;
}

// 将整数处理成字符串并添加到buffer_中
template<typename T>
void LogStream::formatInteger(T num) {
    if(buffer_.valid() >= maxNumberSize){
        char* start = buffer_.current();
        char* cur = start;
        const char* zero = digits + 9;
        bool negative = (num < 0);
        do{
            int remain = static_cast<int>(num % 10);
            *(cur++) = zero[remain];
            num /= 10;
        } while (num != 0);
        if(negative){
            *(cur++) = '-';
        }
        *cur = '\0';
        std::reverse(start,cur);
        buffer_.add(static_cast<int>(cur - start));
    }
}
LogStream& LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(float v)
{
    *this << static_cast<double>(v);
    return *this;
}

LogStream& LogStream::operator<<(double v) {
    if(buffer_.valid() >= maxNumberSize){
        int len = snprintf(buffer_.current(),maxNumberSize,"%0.12g",v);
        buffer_.add(len);
    }
    return *this;
}
LogStream& LogStream::operator<<(char c) {
    buffer_.append(&c,1);
    return *this;
}

LogStream &LogStream::operator<<(const void *p) {
    uintptr_t v = reinterpret_cast<uintptr_t >(p);
    if(buffer_.valid() >= maxNumberSize){
        char *buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';

        size_t len = convertHex(buf+2,v);
        buffer_.add(len + 2);
    }
    return *this;
}
LogStream& LogStream::operator<<(const char* str)
{
    if (str)
    {
        buffer_.append(str, strlen(str));
    }
    else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}
LogStream& LogStream::operator<<(const unsigned char* str)
{
    return operator<<(reinterpret_cast<const char*>(str));
}

LogStream& LogStream::operator<<(const std::string& str)
{
    buffer_.append(str.c_str(), str.size());
    return *this;
}

LogStream& LogStream::operator<<(const SmallBuffer& buf)
{
    *this << buf.toString();        // 调用的是LogStream::operator<<(const std::string& str)
    return *this;
}

LogStream& LogStream::operator<<(const GeneralTemplate& g)
{
    buffer_.append(g.data_, g.len_);
    return *this;
}