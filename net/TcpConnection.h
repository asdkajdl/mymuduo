#pragma once
#include "../base/nocopyable.h"
#include "Callback.h"
#include "Buffer.h"
#include "../base/Timestamp.h"
#include "InetAddress.h"

#include <atomic>
class Channel;
class EventLoop;
class Socket;

//enable_shared_from_this类模板，继承这个类模板后就能调用shared_from_this获得自身的智能指针引用，类似trait编程
class TcpConnection: nocopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop* loop,//？？这里为什么还要接受一个EventLoop?
                  const std::string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddrress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}

    bool connected() const {return state_ == Connected;}

    //发送数据
    void send(const std::string& buf);
    void send(Buffer* buffer);
    void shutdown();

    //保存用户自定义的回调函数
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback &cb)
    { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    //TcpServer会调用
    void connectEstablished();//连接建立
    void connectDestroyed();//连接销毁

private:
    enum State{
        Disconnected,//已经断开连接
        Conncting,
        Connected,
        Disconnecting,
    };
    void setState(State s){state_ = s;}
    //注册到channel上有事件发生时，其回调函数绑定的下面这些函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    //在自己所属的loop中发送数据
    void sendInLoop(const void* message,size_t len);
    void sendInLoop(const std::string& message);
    //在自己所属的Loop中关闭连接
    void shutdownInLoop();


    EventLoop* loop_;//属于哪个subLoop
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;//把fd封装为socket，便于socket析构时自动关闭fd
    std::unique_ptr<Channel> channel_;//fd对应的Channel
    const InetAddress localAddr_;//本地服务器地址
    const InetAddress peerAddr_;
    /**
     * 用户自定义的这些事件的处理函数，然后传递给 TcpServer
     * TcpServer 再在创建 TcpConnection 对象时候设置这些回调函数到TcpConnection中
     */
    ConnectionCallback connectionCallback_;//有新连接时的回调
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完后回调
    CloseCallback closeCallback_;//客户端关闭连接的回调
    HighWaterMarkCallback highWaterMarkCallback_;//超出水位实现的回调

    size_t highWaterMark_;
    Buffer inputBuffer_;//读取数据缓冲区
    Buffer outputBuffer_;//发送数据缓冲
};