#include "EventLoop.h"
#include "Channel.h"
#include "EpollPoller.h"
#include "TimerQueue.h"

#include <sys/eventfd.h>

// 通过__thread 修饰的变量，在线程中地址都不一样，__thread变量每一个线程有一份独立实体，各个线程的值互不干扰.
// t_loopInThisThread的作用：防止一个线程创建多个EventLoop, 是如何防止的呢，请继续读下面注释：
// 当一个eventloop被创建起来的时候,这个t_loopInThisThread就会指向这个Eventloop对象。
// 如果这个线程又想创建一个EventLoop对象的话这个t_loopInThisThread非空，就不会再创建了。
__thread EventLoop* t_loopInthisThread = nullptr;

//定义默认的Epoller IO服用接口的超时时间
const int PollTimeMs = 10000;

int createEventfd(){
    int evfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evfd < 0) {
        LOG_FATAL<<"eventfd error: "<<errno;
    }else {
        LOG_DEBUG<<"create a new wakeupfd, fd: "<<evfd;
    }
    return evfd;
}
EventLoop::EventLoop(): looping_(false),quit_(false), callingPendingFunctors_(false),
threadId_(currentThread::tid()),epoller_(new EpollPoller(this)),timerQueue_(new TimerQueue(this)),
wakeupFd_(createEventfd()),wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG<<"EventLoop created "<<this <<", the thread is "<<threadId_;
    if(t_loopInthisThread){
        LOG_FATAL << "Another EventLoop" << t_loopInthisThread << " exists in this thread " << threadId_;
    }else {
        t_loopInthisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInthisThread = nullptr;
}
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    LOG_INFO <<"EventLoop "<<this<<" start looping";
    while(!quit_){
        activeChannels_.clear();
        epollReturnTime_ = epoller_->poll(PollTimeMs, &activeChannels_);
        for(Channel* channel: activeChannels_){
            channel->handleEvent(epollReturnTime_);
        }
        /**
 * 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
 * accept接收连接 => 将accept返回的connfd打包为Channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
 * mainloop调用queueInLoop将回调加入subloop的pendingFunctors_中（该回调需要subloop执行 但subloop还在epoller_->poll处阻塞）
 * queueInLoop通过wakeup将subloop唤醒，此时subloop就可以执行pendingFunctors_中的保存的函数了
 **/
        // 执行其他线程添加到pendingFunctors_中的函数
        doPendingFunctors();
    }
    looping_ = false;
}
void EventLoop::quit() {
    quit_ = true;
    if(!isInLoopThread()){//如果是另外一个线程运行的quit函数(非创建EventLoop的线程)
        wakeup();
    }
}

void EventLoop::runInLoop(EventLoop::Functor cb) {
    // 如果当前调用runInLoop的线程正好是EventLoop绑定的线程，则直接执行此函数,
    // 否则就将回调函数通过queueInLoop()存储到pendingFunctors_中
    if(isInLoopThread()){
        cb();
    }else {
        queueInLoop(cb);
    }
}
// 把cb放入队列中 唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(EventLoop::Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // callingPendingFunctors_ 比较有必要，因为正在执行回调函数（假设是cb）的过程中，
    // cb可能会向pendingFunctors_中添加新的回调,
    // 则这个时候也需要唤醒，否则就会发生有事件到来但是仍被阻塞住的情况。
    // callingPendingFunctors_在doPendingFunctors函数中被置为true
    if(!isInLoopThread() || callingPendingFunctors_){//不管cb有没有添加新回调，都wakeup，只要在执行中就wakeup
        wakeup();
    }
}
void EventLoop::wakeup() {
    // wakeup() 的过程本质上是对wakeupFd_进行写操作，以触发该wakeupFd_上的可读事件,
    // 这样就起到了唤醒 EventLoop 的作用。
    uint64_t one = 1;
    size_t n = write(wakeupFd_,&one,sizeof(one));
    if(n != sizeof(one)){
        LOG_ERROR<<"EventLoop::wakeup writes "<<n<<" bytes instead of 8";
    }else {
        LOG_DEBUG<<"EventLoop::wakeup writes "<<n;
    }
}
void EventLoop::handleRead() {//只用来处理wakeupfd的读
    uint64_t one = 1;
    size_t n = read(wakeupFd_,&one,sizeof(one));
    if(n != sizeof(one)){
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }else {
        LOG_DEBUG << "success read wakeupFd_";
    }
}

void EventLoop::updateChannel(Channel *channel) {
    epoller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    epoller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return epoller_->hasChannel(channel);
}

void EventLoop::runAt(Timestamp time, EventLoop::Functor &&cb) {
    timerQueue_->addTimer(std::move(cb),time,0.0);
}
void EventLoop::runAfter(double delay, EventLoop::Functor &&cb) {
    Timestamp time(addTime(Timestamp::now(),delay));
    runAt(time,std::move(cb));
}
void EventLoop::runEvery(double interval, EventLoop::Functor &&cb) {
    Timestamp timestamp(addTime(Timestamp::now(),interval));//获取下次运行时间
    timerQueue_->addTimer(std::move(cb),timestamp,interval);//指定运行时间间隔
}
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        // 通过局部变量functors取出pendingFunctors_中的数据，这样可以提前释放锁。
        std::unique_lock<std::mutex> lock(mutex_);
        // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中也要加锁，就会死锁
        functors.swap(pendingFunctors_);
    }
    for(const Functor& f:functors){
        f();
    }
    callingPendingFunctors_ = false;

}
