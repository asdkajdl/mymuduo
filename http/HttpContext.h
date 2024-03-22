#pragma once
#include "HttpRequest.h"

class Buffer;

//解析请求，将解析后的信息存入到HttpRequest类中
class HttpContext{
public:
    //解析状态
    enum HttpRequestParaseState{
        RequestLine,//接下来希望buffer中存的时请求行
        Headers,
        Body,
        All,
    };
    HttpContext():state_(RequestLine){}
    bool paraseRequest(Buffer* buf, Timestamp receiveTime);//解析请求
    bool gotAll()const{return state_ == All;}
    void reset(){
        state_ = RequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }
    const HttpRequest& getRequest() const {return request_;}
    HttpRequest& getRequest(){return request_;}

private:
    //处理请求行
    bool processRequestLine(const char* begin, const char* end);
    HttpRequestParaseState state_;
    HttpRequest request_;
};