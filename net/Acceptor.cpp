
#include "Acceptor.h"
#include "InetAddress.h"
#include "../logger/Logging.h"



static int createNonblocking(){
   int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
   if(sockfd < 0){
       LOG_FATAL<<"listen socket create err"<<errno;
   }
   return sockfd;
}
Acceptor::Acceptor(EventLoop *loop, const InetAddress &ListenAddr, bool reuseport)
    :loop_(loop),
     acceptSocket_(createNonblocking()),
     acceptChannel_(loop,acceptSocket_.fd()),
     listenning_(false),
     idleFd_(::open("/dev/null",O_RDONLY | O_CLOEXEC))
{
    LOG_DEBUG<<"Acceptor create nonblocking socket fd: "<<acceptChannel_.fd();
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(ListenAddr);
//为acceptChannel_的fd绑定可读事件，当有新连接到来时，acceptChannel_的fd可读
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}
Acceptor::~Acceptor() {
    //把从Poller中感兴趣的事件删除掉
    acceptChannel_.disableAll();//将fd感兴趣的事件设为0
    acceptChannel_.remove();//从epoller的映射列表中拿掉，从epfd中拿掉fd
    ::close(idleFd_);
}

void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen();
    //将acceptChannel的读事件注册到epoller
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {//当Epooler发现fd有读事件，就会调用这个函数，这个函数处理listen，有读事件一定是accept的，
//    然后产生新的fd，新fd为通信描述符，会接受信息，。将新fd放入。。
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnectionCallback_){
            //分发连接到subloop
            newConnectionCallback_(connfd,peerAddr);
        }else {
            LOG_DEBUG<<"no newConnectionCallback_ function";
            ::close(connfd);
        }
    }else {
        LOG_ERROR<<"accpet() failed";
        //当前进程的fd已经用完了，可以调整单个服务器的fd上线，也可以分布式部署
        if(errno == EMFILE){
            LOG_INFO<<"sockfd reached limit";
            ::close(idleFd_);
            //接受连接然后立马关闭连接，便于通知客户端能做出响应处理
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            //重新占位
            idleFd_ = ::open("/dev/null",O_RDONLY|O_CLOEXEC);
        }
    }
}