#pragma once
#include "../base/nocopyable.h"
#include "../base/Timestamp.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class EpollPoller : nocopyable{
public:
    using ChannelList = std::vector<Channel*>;
    EpollPoller(EventLoop* loop);
    ~EpollPoller();
//    内部就是调用epoll_wait，将有事件发生的channel通过activeChannels返回
    Timestamp poll(int timeoutMs,ChannelList* activeChannels);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
//    判断channel是否已经注册到了Epoller
    bool hasChannel(Channel* channel);

private:
    using ChannelMap = std::unordered_map<int,Channel*>;
    using EventList = std::vector<epoll_event>;
    // 把所有事件发生的channel添加到activeChannels中
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    //更新channel通道
    void update(int operation,Channel* channel);
    //监听事件数量
    static const int initEventListSize = 16;
    // 储存channel 的映射，（sockfd -> channel*）,即，通过fd快速找到对应的channel
    ChannelMap  channels_;
//定义EPoller所属的事件循环EventLoop
    EventLoop* ownerLoop_;
    int epollfd_;//每个EpollPoller都有一个epollfd_,epollfd_是epoll_create在内核创建空间返回的fd
    //用于存放epoll_wait返回的所有发生事件
    EventList events_;
};