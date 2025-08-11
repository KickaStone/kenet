#ifndef _NET_CHANNEL_H_
#define _NET_CHANNEL_H_

#include <functional>
#include "EventLoop.h"

class Channel {
    public:
        using Callback = std::function<void()>;
        Channel(EventLoop* loop, int fd);
        void setReadCallback(Callback cb);
        void setWriteCallback(Callback cb);
        void setCloseCallback(Callback cb);
        void setErrorCallback(Callback cb);
    
        void enableReading();
        void enableWriting();
        void disableWriting();
        void disableAll();
    
        void handleEvent(uint32_t revents); // 由 Poller 调用
        int fd() const { return fd_; }
        uint32_t events() const { return events_; }
    
    private:
        EventLoop* loop_;
        const int fd_;
        uint32_t events_{0};
        // 回调
        Callback readCb_, writeCb_, closeCb_, errorCb_;
        bool added_{false}; // 是否已在 epoll 中
    };
    

#endif