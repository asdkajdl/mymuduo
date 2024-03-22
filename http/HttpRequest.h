#pragma once

#include "../base/nocopyable.h"
#include "../base/Timestamp.h"
#include <map>

class HttpRequest{
public:
    enum Method{Invalid,Get,Post,Head,Put,Delete};
    enum Version{Unknown,Http10,Http11};
    HttpRequest():method_(Invalid),version_(Unknown){}
    void setVersion(Version v){
        version_ = v;
    }
    Version version() const {return version_;}

    bool setMethod(const char* start, const char* end){
        std::string m(start,end);
        if(m == "GET"){
            method_ = Get;
        }else if(m == "POST"){
            method_ = Post;
        }else if(m == "HEAD"){
            method_ = Head;
        }else if(m == "PUT"){
            method_ = Put;
        }else if(m == "DELETE"){
            method_ = Delete;
        }
        return method_!=Invalid;
    }
    Method method() const {return method_;}
    const char* methodString() const{
        const char* result = "UnKnown";
        if(method_ == Get){
            result = "GET";
        }else if(method_ == Post){
            result = "POST";
        }else if(method_ == Head){
            result = "HDEA";
        }else if(method_ == Put){
            result = "PUT";
        }else if(method_ == Delete){
            result = "DELETE";
        }
        return result;
    }
    void setPath(const char* start, const char* end){
        path_.assign(start,end);
    }
    const std::string getPath() const{
        return path_;
    }
    void setQuery(const char* start, const char* end){
        query_.assign(start,end);
    }
    const std::string getQuery() const{
        return query_;
    }
    void setReceiveTime(Timestamp t){
        receiveTime_ = t;
    }
    Timestamp getReceiveTime() const {return receiveTime_;}
    void addHeader(const char* start,const char* colon, const char* end){
        std::string head(start,colon);
        ++colon;
        while(colon < end && isspace(*colon)){
            colon++;
        }
        std::string value(colon,end);
        while(!value.empty() && isspace(value[value.size() - 1])){
            value.resize(value.size() - 1);
        }
        headers_[head] = value;
    }
    //获取请求头部对应的值
    std::string getHeader(const std::string& head) const{
        std::string result;
        if(headers_.count(head) != 0){
            result = headers_.at(head);
        }
        return result;
    }
    std::map<std::string,std::string>& headers(){return headers_;}//引用返回
    void swap(HttpRequest& rhs){
        path_.swap(rhs.path_);
        query_.swap(rhs.query_);
        std::swap(receiveTime_,rhs.receiveTime_);
        headers_.swap(rhs.headers_);
        std::swap(method_,rhs.method_);
        std::swap(version_,rhs.version_);
    }
private:
    std::string path_;
    std::string query_;
    Timestamp receiveTime_;
    std::map<std::string,std::string> headers_;
    Method method_;
    Version version_;
};