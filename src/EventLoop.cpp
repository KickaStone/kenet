#include "EventLoop.h"
#include "Channel.h"
#include "logger/log.h"


/**
 * @brief loop 函数是EventLoop的主循环函数，eventloop创建后要主动调用loop进行调用。在主循环中，
 */
void EventLoop::loop() {
    quit_.store(false);
    LOG_INFO("EventLoop::loop: start loop in thread: %lu", std::hash<std::thread::id>{}(tid_));
    std::vector<Channel*> activeChannels_;

    while (!quit_.load()) {
        // poller 获取活跃的channel
        auto ret = poller_.poll(pollTimeoutMs_, activeChannels_);
        if (ret == -1) {
            LOG_ERROR("EventLoop::loop: poll failed");
        }
        for (const auto& ch : activeChannels_) {
            threadPool_.enqueue([ch]() {
                ch->handleEvent();
            });
        }
        activeChannels_.clear();
        doPendingFuncs();
    }

    // ensure all pending tasks are executed
    doPendingFuncs();
    LOG_INFO("EventLoop::loop: end loop in thread: %lu", std::hash<std::thread::id>{}(tid_));
}

void EventLoop::quit() {
    quit_.store(true);
}

void EventLoop::runInLoop(std::function<void()> cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(std::function<void()> cb) {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFuncs_.push_back(cb);
}

void EventLoop::doPendingFuncs() {
    // switch pendingFuncs_ with empty vector
    std::vector<std::function<void()>> funcs;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        funcs.swap(pendingFuncs_);
    }
    // Use thread pool to execute the functions
    for (auto& func : funcs) {
        threadPool_.enqueue(func);
    }
}