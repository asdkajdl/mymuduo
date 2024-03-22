#include "EpollPoller.h"
#include "../logger/Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include <assert.h>

const int New = -1;
const int Added = 1;
const int Deleted = 2;

EpollPoller::EpollPoller(EventLoop *loop):ownerLoop_(loop),//记录epollpoller属于哪个eventloop
epollfd_(::epoll_create1(EPOLL_CLOEXEC)),events_(initEventListSize){
    if(epollfd_ < 0){
        LOG_FATAL<<"epoll_create() error: "<<errno;
    }else {
        LOG_DEBUG<<"create a new epollfd, fd = "<<epollfd_;
    }
}
EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}
//epoll_wait
Timestamp EpollPoller::poll(int timeoutMs, EpollPoller::ChannelList *activeChannels) {
    size_t numEvents = ::epoll_wait(epollfd_,&(*events_.begin()),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0){
        fillActiveChannels(numEvents,activeChannels);//填充活跃的channels
        if(numEvents == events_.size()){
            events_.resize(numEvents * 2);//扩容
        }
    }else if(numEvents == 0){//超时
        LOG_TRACE<<"ownerLoop_: "<<ownerLoop_<<" time out!";
    }else {
        //出错
        //不是终端出错
        if(saveErrno != EINTR){
            errno = saveErrno;
            LOG_ERROR<<"EpollPoller::poll() failed";
        }
    }
    return now;
}
void EpollPoller::fillActiveChannels(int numEvents, EpollPoller::ChannelList *activeChannels) const {
    for(int i = 0; i < numEvents; i++){
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);//为什么events里data.ptr是channle*，答：放入events时写入的数据时channel*
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 根据channel在EPoller中的当前状态来更新channel的状态，比如channel还没添加到EPoller中，则就添加进来
void EpollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    if(index == New || index == Deleted){
        int fd = channel->fd();
        if(index == New){
            channels_[fd] = channel;
        }else {//index == Deleted
            assert(channels_.find(fd) != channels_.end());   // 这个处于kDeleted的channel还在channels_中，则继续执行
            assert(channels_[fd] == channel);                // channels_中记录的该fd对应的channel没有发生变化，则继续执行
        }

        channel->set_index(Added);
        update(EPOLL_CTL_ADD,channel);
    }else {// channel已经在poller上注册过,,删除channel
        // 没有感兴趣事件说明可以从epoll对象中删除该channel了
        if(channel->isNonEvent()){
            update(EPOLL_CTL_DEL,channel);
        }else {// 还有事件说明之前的事件删除，但是被修改了
            update(EPOLL_CTL_MOD,channel);//EPOLL_CTL_MOD       修改EPOLL中的事件
        }
    }
}
void EpollPoller::update(int operation, Channel *channel) {
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.events = channel->events();//channel感兴趣的事件
    event.data.ptr = channel;
    int fd = channel->fd();
    if(channels_.count(fd) == 0 && operation == EPOLL_CTL_DEL)return;//如果操作类型为删除而且channel中已经不存在fd了，就返回
    if(::epoll_ctl(epollfd_,operation,fd,&event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR<<"epoll_ctl() delete error: "<<errno;
        }else {
            LOG_FATAL<<"epoll_ctl() add/mod error: "<<errno;
        }
    }
}

// 当连接销毁时，从EPoller移除channel，这个移除并不是销毁channel，而只是把chanel的状态修改一下
// 这个移除指的是EPoller再检测该channel上是否有事件发生了
void EpollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);
    int index = channel->index();
    if(index == Added){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(New);
}
bool EpollPoller::hasChannel(Channel *channel) {//函数不能加const,因为对于不在map中的关键字，使用下标操作符会创建新的条目，改变了map
    int fd = channel->fd();
    return channels_.count(fd)!=0 && channels_[fd] == channel;
}


