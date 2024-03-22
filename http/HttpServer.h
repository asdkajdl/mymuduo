#pragma once

#include "../net/TcpServer.h"
//给出一些回调函数，回调函数要对请求或响应操作

class HttpRequest;

class HttpResponse;

//这里要实现connectCallback，计时器在EvenLoop中，设置函数，调用什么的，但是没有用到
class HttpServer:nocopyable{
public:
    using HttpCallback = std::function<void(const HttpRequest&,HttpResponse*)>;
    HttpServer(EventLoop* loop,const InetAddress& listenAddr,
               const std::string& name,TcpServer::Option option = TcpServer::NoReusePort);
    EventLoop* getLoop() const {return server_.getEventLoop();}
    //实际就是调用用户自定义的onRequest
    void setHttpCallback(const HttpCallback& cb){
        httpCallback_ = cb;
    }
    void setThreadNum(int numThreads){
        server_.setThreadNum(numThreads);
    }
    void start();
private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr&,const HttpRequest&);
    TcpServer server_;
    HttpCallback httpCallback_;

};