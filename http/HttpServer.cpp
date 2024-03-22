#include "HttpServer.h"
#include "../logger/Logging.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

/**
* 默认http回调函数
 * 设置响应状态码，相应信息并关闭连接
*/
void defaultHttpCallback(const HttpRequest&,HttpResponse* resp){
    resp->setStatusCode(HttpResponse::NotFound404);
    resp->setStatusMsg("Not Found");
    resp->setCloseConnection(true);
}
HttpServer::HttpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name,
                       TcpServer::Option option)
                       : server_(loop,listenAddr,name,option),
                       httpCallback_(defaultHttpCallback){
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection,this,std::placeholders::_1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
//    没有,但是都写完了,应该也是就记录一下日志信息.writeCompleteCallback_
}
void HttpServer::start() {
    server_.start();
}
void HttpServer::onConnection(const TcpConnectionPtr &conn) {
    if(conn->connected()){
        LOG_DEBUG << "new Connection arrived";//我就说，新连接的时候newConnection已经做了很多了，都做完了，他就只是结束
    }else {
        LOG_DEBUG<<"Connection closed, name = "<<conn->name();
    }
}
void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime) {//来消息的时候执行这个回调
    LOG_INFO<<"onMessage conn loop "<<conn->getLoop()<<"in Buffer message "<<buf->peek();
    std::unique_ptr<HttpContext> context(new HttpContext);
    if(!context->paraseRequest(buf,receiveTime)){
        LOG_INFO<<"this conn belongs to the loop: "<<conn->getLoop()<<"paraseRequest failed";
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");//正常应该发送response
        conn->shutdown();
    }
    if(context->gotAll()){//解析完了
        LOG_INFO<<"this conn belongs to the loop: "<<conn->getLoop()<<"paraseRequest success";
        onRequest(conn,context->getRequest());
        context->reset();
    }
}
void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req) {
    const std::string& connection = req.getHeader("Connection");
    bool close = connection == "close" || (req.version() == HttpRequest::Http10 && connection!="Keep-Alive");
    HttpResponse response(close);
    httpCallback_(req,&response);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if(response.closeConnection()){
        LOG_DEBUG<<"the server close http connection, named: "<<conn->name();
        conn->shutdown();
    }
}