#include "TcpConnection.h"
#include "../logger/Logging.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>


// 提供一个默认的ConnectionCallback，如果自定义的服务器（比如EchoServer）没有注册
// ConnectionCallback的话，就使用这个默认的(声明在Callback.h)
void defaultConnectionCallback(const TcpConnectionPtr& conn){
    LOG_TRACE<<conn->localAddrress().toIpPort()<<"->"<<conn->peerAddress().toIpPort()<<" is "<<(conn->connected()?"up":"down");
}
// 提供一个默认的MessageCallback，如果自定义的服务器（比如EchoServer）没有注册
// MessageCallback的话，就使用这个默认的(声明在Callback.h)
void defaultMessageCallback(const TcpConnectionPtr&,Buffer* buffer,Timestamp){//实际应该拿去解析HTTP，然后写数据
    LOG_TRACE<<"=================================";
    LOG_TRACE<<"bytes "<<buffer->readableBytes()<<"receive "<<buffer->retrieveAllAsString();
}
static EventLoop* CheckLoopNotNull(EventLoop* loop){
    // 如果传入EventLoop没有指向有意义的地址则出错
    // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
    if(loop == nullptr){
        LOG_FATAL <<"mainLoop is nullptr";
    }
    return loop;
}
TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,const InetAddress &peerAddr)
    :loop_(CheckLoopNotNull(loop)),
    name_(name),
    state_(Conncting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop,sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64 * 1024 * 1024)//64M
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError,this));
    LOG_INFO<<"TcpConnetction::creator["<<name_.c_str()<<"] at fd = "<<sockfd;
    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection() {
    LOG_INFO<<"TcpConnetction::deletor["<<name_.c_str()<<"] at fd = "<<channel_->fd()<<"state="<<static_cast<int>(state_);
}
void TcpConnection::send(const std::string& buf) {
    if(state_ == Connected){
        if(loop_->isInLoopThread()){
            sendInLoop(buf.c_str(),buf.size());
        }else {
            loop_->runInLoop(std::bind((void(TcpConnection::*)(const std::string&))&TcpConnection::sendInLoop,this,buf));
//            (void(TcpConnection::*)(const std::string&))：这部分表示成员函数指针的类型，它指定了成员函数的签名。在这个例子中，它表示一个属于 TcpConnection 类的
//            成员函数指针，该成员函数接受一个 const std::string& 类型的参数，并返回 void。这种语法告诉编译器我们正在处理一个成员函数指针，并且指定了函数的参数和返回类型。
//             夸张，因为sendInLoop是重载寒湖是，所以指明了指针的类型，类成员指针+参数类型+发挥至
        }
    }
}
void TcpConnection::send(Buffer *buffer) {
    if(state_ == Connected){
        if(loop_->isInLoopThread()){
            sendInLoop(buffer->retrieveAllAsString());
        }else {
            std::string msg = buffer->retrieveAllAsString();
            loop_->runInLoop(std::bind((void(TcpConnection::*)(const std::string&))&TcpConnection::sendInLoop,this,msg));
        }
    }
}

void TcpConnection::sendInLoop(const std::string &message) {
    sendInLoop(message.c_str(),message.size());
}
/**
 * 在当前所属loop中发送数据data。发送前，先判断outputBuffer_中是否还有数据需要发送，如果没有，
 * 则直接把要发送的数据data发送出去。如果有，则把data中的数据添加到outputBuffer_中，并注册可
 * 写事件
 **/
void TcpConnection::sendInLoop(const void *message, size_t len) {//真正发送数据的函数
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if(state_ == Disconnected){
        LOG_ERROR<<"disconnected give up writing";
        return ;
    }
    // 当channel_没有注册可写事件并且outputBuffer_中也没有数据要发送了，则直接*data中的数据发送出去
    // 疑问：什么时候isWriting返回false?
    // 答：刚初始化的channel和数据发送完毕的channel都是没有可写事件在epoll上的,即isWriting返回false，
    // 对于后者，见本类的handlWrite函数，发现只要把数据发送完毕，他就注销了可写事件
    LOG_DEBUG<<"this loop "<<loop_<<" the message in sendInLoop  is "<<message;
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
        nwrote = ::write(channel_->fd(),message,len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }else {
            nwrote = 0;
            if(errno != EWOULDBLOCK){//EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回
                LOG_ERROR<<"TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
        if(!faultError && remaining > 0){
            size_t oldLen = outputBuffer_.readableBytes();// 目前发送缓冲区剩余的待发送的数据的长度
            //判断待写数据是否会超过设置的高位标志highWaterMark_
            if(oldLen + remaining > highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
                loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this()));//我怀疑真正的从buffer发送数据是highWaterMarkCallback_函数
            }
            outputBuffer_.append((char*)message + nwrote,remaining);
            if(!channel_->isWriting()){
                channel_->enableWriting();//// 这里一定要注册channel的写事件 否则Epoller不会给channel通知epollout
            }
        }
    }
}

void TcpConnection::shutdown() {
    if(state_ == Connected){
        setState(Disconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,shared_from_this()));
    }
}
void TcpConnection::shutdownInLoop() {
    if(!channel_->isWriting()){//说明当前outputBuffer_的数据全部向外发送完成
        socket_->shutdownWrite();
    }
}
//连接建立
void TcpConnection::connectEstablished() {
    setState(Connected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    LOG_TRACE<<"connectEstablished executed channel enableReading... "<<channel_->isReading();
    //新连接建立，执行回调，其实就只是记录了信息

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    if(state_ == Connected){
        setState(Disconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());//又执行一遍??，因为啥都没做，所以又执行了一遍。记录连接断开的信息
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {//真正的读
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveErrno);
    LOG_INFO<<"read data from channel->fd "<<inputBuffer_.peek()<<"size "<<n;
    if(n > 0){//// 从fd读到了数据，并且放在了inputBuffer_上
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n == 0){
        //对方关闭了连接
        handleClose();
    }else {
        //出错
        errno = saveErrno;
        LOG_ERROR<<"Tcpconnection::handleRead() failed";
        handleError();
    }
}
void TcpConnection::handleWrite() {
    if(channel_->isWriting()){
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n > 0){
            // 把outputBuffer_的readerIndex往前移动n个字节，因为outputBuffer_中readableBytes已经发送出去了n字节
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0){
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(state_ == Disconnecting){//Disconnecting没见到这个标志
                    shutdownInLoop();
                }
            }
        }else {
            LOG_ERROR<<"TcpConnection::handleWrite() failed";
        }
    }else {//channel对写不感兴趣就是Buffer写完了
        LOG_ERROR<<"Tcpconnection fd = "<<channel_->fd()<<"is down,no more writing";
    }
}

void TcpConnection::handleClose() {
    setState(Disconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);//connectionCallback_到底干了什么，怎么关闭还要调用这个  执行连接关闭的回调(用户自定的，而且和新连接到来时执行的是同一个回调)
    closeCallback_(connPtr);// 执行关闭连接的回调 执行的是TcpServer::removeConnection回调方法
}
void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    //《Linux高性能服务器编程》page88，获取并清除socket错误状态,getsockopt成功则返回0,失败则返回-1
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0){
        err = errno;
    }else {
        err = optval;
    }
    LOG_ERROR<<"TcpConnection::handleError name: "<<name_.c_str()<<" - SO_ERROR: "<<err;
}