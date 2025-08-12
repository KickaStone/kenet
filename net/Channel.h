#ifndef _NET_CHANNEL_H_
#define _NET_CHANNEL_H_

#include <functional>
#include <cstdint>
#include <sys/epoll.h>

// 前向声明
class EventLoop;

class Channel {
    public:
        using Callback = std::function<void()>;
        Channel(EventLoop* loop, int fd);
        void setReadCallback(Callback cb);
        void setWriteCallback(Callback cb);
        void setCloseCallback(Callback cb);
        void setErrorCallback(Callback cb);

        void enableReading() { events_ |= EPOLLIN; update(); }
        void enableWriting() { events_ |= EPOLLOUT; update(); }
        void disableWriting() { events_ &= ~EPOLLOUT; update(); }
        void disableAll() { events_ = 0; update(); }
        void update();
    
        void handleEvent(uint32_t revents); // 由 Poller 调用
        int fd() const { return fd_; }
        uint32_t events() const { return events_; }
        uint32_t revents() const { return revents_; }
        void setEvents(uint32_t events);
        void setRevents(uint32_t revents) { revents_ = revents; }
        void setAdded(bool added) { added_ = added; }
        bool added() const { return added_; }

    private:
        EventLoop* loop_;
        const int fd_;
        uint32_t events_{0};
        uint32_t revents_{0};
        // 回调
        Callback readCb_, writeCb_, closeCb_, errorCb_;
        bool added_{false}; // 是否已在 epoll 中
    };
    

#endif