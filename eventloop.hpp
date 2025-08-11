
#include <functional>
#include "poller.hpp"
#include "common.hpp"


class EventLoop {
    public:
        void loop();        // while(!quit_) { poller_.poll(...); dispatch(); runPendingFunctors(); timers_.run(); }
        void quit();
        void runInLoop(F&& f);
        void queueInLoop(F&& f); // 使用 eventfd 唤醒
        TimerId runAfter(Duration, Functor);
        TimerId runEvery(Duration, Functor);
    private:
        Poller poller_;
        // TimerQueue timers_;
        int wakeupFd_;
        // lock-free MPSC or mutex queue for pending functors
    };