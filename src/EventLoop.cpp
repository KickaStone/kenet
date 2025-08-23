#include "EventLoop.h"
#include "Channel.h"
#include "logger/log.h"

EventLoop::EventLoop()
    : poller_(), quit_(false) {}


// eventloop will block the thread until quit
void EventLoop::loop() {
    quit_.store(false);
    LOG_INFO("EventLoop::loop: start loop in thread: %lu", std::hash<std::thread::id>{}(tid_));
    std::vector<Channel*> activeChannels_;

    while (!quit_.load()) {
        poller_.poll(pollTimeoutMs_, activeChannels_);

        for (auto ch : activeChannels_) {
            ch->handleEvent(ch->revents());
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