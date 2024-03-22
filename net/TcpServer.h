#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "../base/nocopyable.h"
#include "Callback.h"
#include "TcpConnection.h"

#include <unordered_map>
class TcpServer{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option{
        NoReusePort,
        ReusePort,
    };
    TcpServer(EventLoop* loop,const InetAddress& ListenAddr,const std::string& name,Option option = NoReusePort);
    ~TcpServer();
    // 设置回调函数(用户自定义的函数传入)
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void setThreadNum(int numThreads);//设置底层subLoop的个数
    void start();
    EventLoop* getEventLoop() const {return loop_;}

    const std::string getName() const {return name_;}
    const std::string ipPort() const {return ipPort_;}
private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* loop_;//用户定义的mainloop
    const std::string ipPort_;//传入的Ip地址和端口号
    const std::string name_;//TcpServer的名字
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;//有读写消息时的回调函数
    WriteCompleteCallback writeCompleteCallback_;//消息发送完成以后的回调函数
    ThreadInitCallback threadInitCallback_;//loop线程初始化的回调函数
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_;

};