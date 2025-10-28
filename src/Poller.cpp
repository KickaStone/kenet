#include "Poller.h"
#include "Channel.h"
#include "logger/log.h"

/**
 * 构建Poller时调用epoll_create1创建epoll文件描描述符。设置为EPOLL_CLOEXEC主要是提高安全性。在CLOEXEC模式下，进程fork或调用exec执行一个新程序时，内核自动关闭文件描述符。
 */
Poller::Poller()
{
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epfd_ == -1) {
        LOG_FATAL("epoll_create1 failed");
        exit(1);
    }
}

Poller::~Poller() {
    close(epfd_);
}

/**
 * 使用epoll_ctl删除监听的channel对应的fd。
 * @param ch 要删除的channel。
 */
void Poller::removeChannel(const std::shared_ptr<Channel>& ch) const {
    // 从epoll中删除fd
    epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), nullptr);
    ch->setAdded(false);
}

/**
 * 使用epoll_ctl添加或修改监听的channel对应的fd。
 * @param ch 要添加或修改的channel。
 */
void Poller::updateChannel(const std::shared_ptr<Channel>& ch) const {
    epoll_event ev{};
    ev.events = ch->events();
    ev.data.ptr = ch.get();  // channel data pointer
    
    if (ch->added()) {
        // modify existing fd
        epoll_ctl(epfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    } else {
        // add new fd
        epoll_ctl(epfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
    }
    ch->setAdded(true);
}

/**
 * 收集epoll_wait返回的活跃事件，并将其添加到输入参数的活跃channel列表中。
 * @param timeoutMs 等待事件的超时时间，单位为毫秒。
 * @param active 活跃channel列表。
 * @return 返回收集到的活跃事件数量。
 *         -1 表示发生错误。
 *         0 表示没有事件发生。
 *         大于0 表示收集到的活跃事件数量。
 */
int Poller::poll(int timeoutMs, std::vector<Channel*>& active) {
    if (timeoutMs <= 0) {
        timeoutMs = -1; // 如果timeoutMs小于等于0，则设置为-1，表示无限等待。
    }
    int nfds = epoll_wait(epfd_, events_, MAX_EVENTS, timeoutMs);
    if (nfds < 0) {
        LOG_ERROR("epoll_wait failed");
        return -1;
    }
    for (int i = 0; i < nfds; ++i) {
        LOG_INFO("Poller::poll: event %d: %d", i, events_[i].events);
        auto* ch = reinterpret_cast<Channel*>(events_[i].data.ptr);
        ch->setRevents(events_[i].events);
        active.push_back(ch);
    }
    return nfds;
}