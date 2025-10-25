#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include <functional>
#include <vector>

#include <atomic>
#include <thread>
#include <mutex>

#include "Channel.h"
#include "Poller.h"
#include "threadpool/threadpool.h"

/**
 * @brief Eventloop是epoll的事件循环
 */
class EventLoop {
    public:
        EventLoop() = default;
        void loop();
        void quit();                // 设置 quit_ = true
        [[nodiscard]] bool isInLoopThread() const { return std::this_thread::get_id() == tid_; }
        void runInLoop(std::function<void()> cb);
        void queueInLoop(std::function<void()> cb);
        Poller& poller() { return poller_; }
        void updateChannel(Channel* ch) const { poller_.updateChannel(ch->shared_from_this()); }
        void removeChannel(Channel* ch) const { poller_.removeChannel(ch->shared_from_this()); }
    private:
        Poller poller_;
        std::atomic<bool> quit_{false};
        const std::thread::id tid_ = std::this_thread::get_id(); // 构造时抓取
        std::mutex mutex_;
        std::vector<std::function<void()>> pendingFuncs_;
        static constexpr int pollTimeoutMs_ = 1000;
        void doPendingFuncs();

        ThreadPool threadPool_{4ul};
};
#endif