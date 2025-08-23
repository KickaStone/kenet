#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include <functional>
#include <vector>

#include <atomic>
#include <thread>
#include <mutex>
#include "Poller.h"

class EventLoop {
    public:
        EventLoop();
        void loop();
        void quit();                // 设置 quit_ = true
        bool isInLoopThread() const { return std::this_thread::get_id() == tid_; }
        void runInLoop(std::function<void()> cb);
        void queueInLoop(std::function<void()> cb);
        Poller& poller() { return poller_; }
        void updateChannel(Channel* ch) { poller_.updateChannel(ch); }
        void removeChannel(Channel* ch) { poller_.removeChannel(ch); }
    private:
        Poller poller_;
        std::atomic<bool> quit_{false};
        const std::thread::id tid_ = std::this_thread::get_id(); // 构造时抓取
        std::mutex mutex_;
        std::vector<std::function<void()>> pendingFuncs_;
        static const int pollTimeoutMs_ = 1000;
        void doPendingFuncs();
    };
    

#endif