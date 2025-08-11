#ifndef _NET_EVENTLOOP_H_
#define _NET_EVENTLOOP_H_

#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

#include "Poller.h"

class EventLoop {
    public:
        void loop();
        void quit();                // 设置 quit_ = true
        bool isInLoopThread() const;
        Poller& poller() { return poller_; }
    private:
        Poller poller_;
        std::atomic<bool> quit_{false};
        const std::thread::id tid_ = std::this_thread::get_id(); // 构造时抓取
    };
    

#endif