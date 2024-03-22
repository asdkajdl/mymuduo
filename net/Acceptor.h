#pragma once

#include "../base/nocopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
/**
 * Acceptor运行在baseLoop中
 * TcpServer发现Acceptor有一个新连接，则将此channel分发给一个subLoop
 */

class EventLoop;
class InetAddress;
class Acceptor{
public:
//    接受新连接的回调函数
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& ListenAddr, bool reuseport);\
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback cb){
        newConnectionCallback_ = cb;
    }
    bool listenning()const {return listenning_;}
    void listen();
private:
    void handleRead();

    EventLoop* loop_;//acceptor使用的是用户定义的mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
    int idleFd_;//防止连接fd数量超过上限，用于占位的fd
};