#include "TcpServer.h"

//检查传入的baseLoop指针是否有意义
static EventLoop* CheckLoopNotNull(EventLoop* loop){
    if(nullptr){
        LOG_FATAL<<"main loop is null";
    }
    return loop;
}
TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name,
                     TcpServer::Option option)
                     :loop_(CheckLoopNotNull(loop)),
                     name_(name),
                     ipPort_(listenAddr.toIpPort()),
                     acceptor_(new Acceptor(loop,listenAddr,option == ReusePort)),
                     threadPool_(new EventLoopThreadPool(loop,name)),
                     connectionCallback_(defaultConnectionCallback),
                     messageCallback_(defaultMessageCallback),
                     writeCompleteCallback_(),
                     threadInitCallback_(),
                     started_(0),
                     nextConnId_(1)
                     {
                         // 当有新用户连接时，Acceptor类中绑定的acceptChannel_会有读事件发生执行handleRead()调用TcpServer::newConnection回调
                         acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));

}
TcpServer::~TcpServer() {
    for(auto &item:connections_){
        TcpConnectionPtr conn(item.second);
        // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
        item.second.reset();
        //销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}
//设置底层subLoop的个数
void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    if(started_++ == 0){
        threadPool_->start(threadInitCallback_);
//        // acceptor_.get()绑定时候需要地址
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}
// 有一个新用户连接，acceptor会执行这个回调操作，负责将mainLoop接收到的请求连接(acceptChannel_会有读事件发生)通过回调轮询分发给subLoop去处理
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {//是accept获得的sockfd和对方地址后调用的newConnection
    //轮询算法，选择一个subLoop来管理connfd对应的channel
    EventLoop* ioLoop = threadPool_->getNextLoop();
    //提示信息
    char buf[64] = {0};
    snprintf(buf, sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    //因为只在mainloop中执行，不涉及安全问题，所以不用设nextConnId为原子类
    nextConnId_++;
    //新连接名字
    std::string connName = name_ + buf;
    LOG_INFO<<"TcpServer::newConnection ["<<name_.c_str()<<"]-new connection ["<<connName.c_str()<<"] from"<<peerAddr.toIpPort().c_str();

    //通过sockfd获取绑定的本机的ip地址和端口信息
    sockaddr_in local;//?新连接到来，为什么要一个local地址呢,因为要创建新的连接
    ::memset(&local,0, sizeof(local));
    socklen_t addrlen = sizeof(local);
//    getsockname()函数用于获取一个套接字的名字。它用于一个已捆绑或已连接套接字s，
//    本地地址将被返回。本调用特别适用于如下情况：未调用bind()就调用了connect()，
//    这时唯有getsockname()调用可以获知系统内定的本地地址。在返回时，namelen参数包含了名字的实际字节数。
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen) < 0){
        LOG_ERROR<<"sockets::getLocalAddr() failed";
    }
    InetAddress localAddr(local);
    //新建一个连接
    TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);//TcpConnectionPtr调用的connectionCallback和accept调用的NewConnectionCallback不是一个东西，tcp链接调用的是上层的
    conn->setMessageCallback(messageCallback_);//onmessage
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {//真正执行移除连接
    LOG_INFO<<"TcpServer::removeConnectionInLoop["<<name_.c_str()<<"]-connection"<<conn->name().c_str();
    connections_.erase(conn->name());
    EventLoop* loop = conn->getLoop();
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}