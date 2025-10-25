#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_

#include <functional>
#include <memory>

#include "EventLoop.h"
#include "Channel.h"
#include "InetAddress.h"

class Acceptor : public std::enable_shared_from_this<Acceptor>
{
public:
    using NewConnFn = std::function<void(int fd, const InetAddress &peer)>;
    Acceptor(std::shared_ptr<EventLoop> loop, const InetAddress &addr);
    void setNewConnCallback(NewConnFn fn) { cb_ = std::move(fn); }
    void Listen();
private:
    void handleAccept();               // accept connections until EAGAIN
    std::shared_ptr<EventLoop> loop_;                  // eventloop
    InetAddress addr_;                 // listen address
    int listenfd_;                     // listen socket fd
    std::shared_ptr<Channel> channel_; // channel to accept connections
    NewConnFn cb_;                     // new connection callback
};

#endif