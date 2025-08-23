#ifndef _NET_ACCEPTOR_H_
#define _NET_ACCEPTOR_H_

#include <functional>
#include <memory>

#include "EventLoop.h"
#include "Channel.h"
#include "InetAddress.h"

class Acceptor
{
public:
    using NewConnFn = std::function<void(int fd, const InetAddress &peer)>;
    Acceptor(EventLoop *loop, const InetAddress &addr);
    void setNewConnCallback(NewConnFn fn) { cb_ = std::move(fn); }
    void Listen();
private:
    void handleAccept();               // accept connections until EAGAIN
    EventLoop *loop_;                  // eventloop
    InetAddress addr_;                 // listen address
    int listenfd_;                     // listen socket fd
    std::unique_ptr<Channel> channel_; // channel to accept connections
    NewConnFn cb_;                     // new connection callback
};

#endif