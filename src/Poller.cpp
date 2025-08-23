#include "Poller.h"
#include "Channel.h"
#include "logger/log.h"

Poller::Poller()
{
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epfd_ == -1) {
        LOG_INFO("epoll_create1 failed");
        exit(1);
    }
}

Poller::~Poller() {
    close(epfd_);
}

void Poller::removeChannel(Channel* ch) {
    // 从epoll中删除fd
    epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), nullptr);
    // 从fd2ch_中删除fd
    fd2ch_.erase(ch->fd());
}

void Poller::updateChannel(Channel* ch) {
    epoll_event ev{};
    ev.events = ch->events();
    ev.data.ptr = ch;
    
    if (ch->added()) {
        // modify existing fd
        epoll_ctl(epfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
    } else {
        // add new fd
        epoll_ctl(epfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
        ch->setAdded(true);
    }
    
    fd2ch_[ch->fd()] = ch;
}


// 将epfd上的活跃事件收集到active中
int Poller::poll(int timeoutMs, std::vector<Channel*>& active) {
    int nfds = epoll_wait(epfd_, events_, MAX_EVENTS, timeoutMs);
    if (nfds < 0) {
        LOG_ERROR("epoll_wait failed");
        return -1;
    }
    for (int i = 0; i < nfds; ++i) {
        Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
        ch->setRevents(events_[i].events);
        active.push_back(ch);
    }
    return nfds;
}