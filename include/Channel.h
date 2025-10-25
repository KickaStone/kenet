#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <functional>
#include <cstdint>
#include <memory>
#include <sys/epoll.h>

// 前向声明
class Poller;

/**
 * @brief Channel类是对文件描述符的封装，一个Channel对应一个文件描述符fd。同时Channel类也注册了该fd上的事件回调函数。
 */
class Channel : public std::enable_shared_from_this<Channel>{
    public:
        using Callback = std::function<void()>;
        Channel(Poller* poller, int fd);
        ~Channel();
        void setReadCallback(Callback cb);
        void setWriteCallback(Callback cb);
        void setCloseCallback(Callback cb);
        void setErrorCallback(Callback cb);

        void enableReading() { events_ |= EPOLLIN; update(); }
        void enableWriting() { events_ |= EPOLLOUT; update(); }
        void disableWriting() { events_ &= ~EPOLLOUT; update(); }
        void disableAll() { events_ = 0; update(); }
        void update();
    
        void handleEvent() const; // 由 Poller 调用
        int fd() const { return fd_; }
        uint32_t events() const { return events_; }
        uint32_t revents() const { return revents_; }
        void setEvents(uint32_t events);
        void setRevents(uint32_t revents) { revents_ = revents; }
        void setAdded(bool added) { added_ = added; }
        bool added() const { return added_; }
        void tie(const std::shared_ptr<void> &owner) { tie_ = owner; }

    private:
        /**
         * @brief 拥有该Channel的Poller。
         */
        Poller* poller_;
        /**
         * @brief 该Channel对应的文件描述符fd。
         */
        const int fd_;
        /**
         * @brief 该Channel上注册的事件。
         */
        uint32_t events_{0};
        /**
         * @brief 该Channel上发生的事件。
         */
        uint32_t revents_{0};
        /**
         * @brief 该Channel上注册的读事件回调函数。
         */
        Callback readCb_, writeCb_, closeCb_, errorCb_;
        bool added_{false}; // 是否已在 epoll 中
        std::weak_ptr<void> tie_;
    };
    

#endif