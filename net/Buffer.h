#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stddef.h>
#include <assert.h>
// 网络库底层的缓冲区类型定义
/*
Buffer的样子！！！
+-------------------------+----------------------+---------------------+
|    prependable bytes    |    readable bytes    |    writable bytes   |
|                         |      (CONTENT)       |                     |
+-------------------------+----------------------+---------------------+
|                         |                      |                     |
0        <=           readerIndex     <=     writerIndex             size
*/
class Buffer{
public:
    static const size_t cheapPrepend = 8;//记录数据包长度的长度
    static const size_t initialSize = 1024;
    explicit Buffer(size_t size = initialSize):buffer_(cheapPrepend + size),readerIndex_(0),writerIndex_(0){}
    size_t readableBytes() const {return writerIndex_ - readerIndex_; }
    size_t writableByte() const {return buffer_.size() - writerIndex_;}
    size_t prependableBytes() const {return readerIndex_;}

    //返回缓冲区可读数据的起始地址
    const char* peek() const {return begin()+readerIndex_;}

    //查找bufer是否有\r\n
    const char* findCRLF() const{
        const char* crlf = std::search(peek(),beginWrite(),CRLF,CRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }
    const char* beginWrite() const{
        return begin() + writerIndex_;
    }
    char* beginWrite(){return begin() + writerIndex_;}
    // 一直取到end位置. 在解析请求行的时候从buffer中读取一行后要移动指针便于读取下一行
    void retrieveUntil(const char* end){//重置下标，将readindex重置到end这里
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    void retrieve(size_t len){
        if(len < readableBytes()){//没读完
            readerIndex_ += len;
        }else {//读完了
            retrieveAll();
        }
    }
    void retrieveAll(){
        writerIndex_ = cheapPrepend;
        readerIndex_ = cheapPrepend;
    }
    // 把onMessage函数上报的Buffer数据 转成string类型的数据返回
    std::string retrieveAllAsString(){return retrieveAsString(readableBytes());}
    std::string retrieveAsString(size_t len){
        std::string s(peek(),len);
        retrieve(len);
        return s;
    }
    // 通过扩容或者其他操作确保可写区域是写的下将要写的数据（具体操作见makeSpace方法）
    void ensurenWritableBytes(size_t len){
        if(len > writableByte()){
            makeSpace(len);
        }
    }
    void append(const std::string& str){
        append(str.data(),str.size());
    }
    void append(const char* data,size_t len){
        ensurenWritableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_ += len;
    }
    ssize_t readFd(int fd, int *saveError);
    ssize_t writeFd(int fd, int *saveError);
private:
    void makeSpace(size_t len){
        if(writableByte() + prependableBytes() < len + cheapPrepend){
            buffer_.resize(writerIndex_ + len);
        }else {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,beginWrite() + writableByte(),begin() + cheapPrepend);
            readerIndex_ = cheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }
    char* begin(){ return &*buffer_.begin();}//buffer_begin为迭代器类型指针
    const char* begin() const {return &*buffer_.begin();}

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char CRLF[];//'\r\n' ///定位request报文
};