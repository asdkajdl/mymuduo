#pragma once

#include "../base/nocopyable.h"

class InetAddress;

//封装socket fd
class Socket:nocopyable{
public:
    explicit Socket(int socketfd):sockefd_(socketfd){}
    ~Socket();

    int fd() const {return sockefd_;}
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite();
    void setTcpNoDely(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockefd_;
};