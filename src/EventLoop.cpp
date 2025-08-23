#include "../include/EventLoop.h"
#include "../include/Channel.h"
#include "../include/logger/log.h"

EventLoop::EventLoop()
    : poller_(), quit_(false) {}

void EventLoop::loop() {
    LOG_INFO("EventLoop::loop: start loop in thread: %lu", std::hash<std::thread::id>{}(tid_));
    std::vector<Channel*> activeChannels_;

    while (!quit_.load()) {
        poller_.poll(1000, activeChannels_);

        for (auto ch : activeChannels_) {
            ch->handleEvent(ch->revents());
        }
        activeChannels_.clear();
        doPendingFuncs();
    }

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
    cond_.notify_one(); // wake up loop
}

void EventLoop::doPendingFuncs() {
    std::vector<std::function<void()>> funcs;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        funcs.swap(pendingFuncs_);
    }
    for (auto& func : funcs) {
        func();
    }
}