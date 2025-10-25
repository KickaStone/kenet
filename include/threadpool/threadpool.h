#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

#include "logger/log.h"

class ThreadPool {
    public:
    explicit ThreadPool();
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    // thread pool
    std::vector<std::thread> workers_;
    // task queue, task packaged as void function
    std::queue<std::function<void()>> tasks_;

    // sync
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic_bool stop_;
};

inline ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}

inline ThreadPool::ThreadPool(size_t threads) : stop_(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    this->condition_.wait(lock, [this]{ return this->stop_.load() || !this->tasks_.empty();});
                    if (this->stop_.load() && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                try {
                    task();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception occurred during task()", e);
                } catch (...) {
                    // 放置线程退出
                    LOG_ERROR("catch exception");
                }
            }
        });
    }
}

inline ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable())
            worker.join();
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::lock_guard lock(mutex_);
        if (stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks_.emplace([task]() { (*task)(); });
    }
    condition_.notify_one();
    return res;
}

# endif
