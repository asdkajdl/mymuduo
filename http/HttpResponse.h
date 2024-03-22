#pragma once

#include <map>
#include <string>
class Buffer;

class HttpResponse{
public:
    enum HttpStatusCode{
        Unknown,
        Ok200 = 200,
        MovePermanently301 = 301,
        BadRequest400 = 400,
        NotFound404 = 404,
    };
    explicit HttpResponse(bool close):statusCode_(Unknown),closeConnection_(close){

    }
    void setStatusCode(HttpStatusCode code){
        statusCode_ = code;
    }
    void setStatusMsg(std::string msg){
        statusMsg_ = msg;
    }
    void setCloseConnection(bool on){
        closeConnection_ = on;
    }
    bool closeConnection() const {return closeConnection_;}
    void setContentType(const std::string& contentType){
        addHeader("Content-Type",contentType);
    }
    void addHeader(const std::string& head,const std::string value){
        headers_[head] = value;
    }
    void setBody(const std::string& body){
        body_ = body;
    }
    void appendToBuffer(Buffer* output)const;
private:
    std::map<std::string,std::string> headers_;
    HttpStatusCode statusCode_;
    std::string statusMsg_;
    bool closeConnection_;
    std::string body_;
};