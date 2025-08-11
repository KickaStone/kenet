#ifndef _NET_ACCEPTOR_H_
#define _NET_ACCEPTOR_H_

#include <functional>
#include <memory>

#include "EventLoop.h"
#include "Channel.h"
#include "InetAddress.h"

class Acceptor {
    public:
        using NewConnFn = std::function<void(int fd, const InetAddress& peer)>;
        Acceptor(EventLoop* loop, const InetAddress& addr);
        void setNewConnCallback(NewConnFn fn) { cb_ = std::move(fn); }
        void listen(); // bind/listen + channel.enableReading()
    private:
        void handleRead(); // accept 直到 EAGAIN
        EventLoop* loop_;
        int listenfd_;
        Channel channel_;
        NewConnFn cb_;
    };
    
#endif