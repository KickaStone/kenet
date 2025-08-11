#ifndef _NET_TCPSERVER_H_
#define _NET_TCPSERVER_H_

#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"


class TcpServer {
    public:
        TcpServer(EventLoop* loop, const InetAddress& addr);
        void setMessageCallback(TcpConnection::MessageCb cb){ onMessage_ = std::move(cb); }
        void setConnectionCallback(TcpConnection::EventCb cb){ onConn_ = std::move(cb); }
        void start(); // acceptor.listen()
    
    private:
        void onNewConn(int fd, const InetAddress& peer);
        void removeConn(const TcpConnection::Ptr& c);
    
        EventLoop* loop_;
        Acceptor acceptor_;
        std::unordered_map<int, TcpConnection::Ptr> conns_;
        TcpConnection::MessageCb onMessage_;
        TcpConnection::EventCb   onConn_;
    };
    

#endif