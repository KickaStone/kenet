#ifndef _NET_POLLER_H_
#define _NET_POLLER_H_

#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>

#include "Logger.h"

// 前向声明
class Channel;

class Poller {
    public:
        explicit Poller();
        ~Poller();
        void updateChannel(Channel* ch);  // ADD/MOD/DEL
        void removeChannel(Channel* ch);
        int poll(int timeoutMs, std::vector<Channel*>& active);
    private:
        int epfd_;
        std::unordered_map<int, Channel*> fd2ch_; // fd -> Channel*
        static const int MAX_EVENTS = 1024;
        epoll_event events_[MAX_EVENTS];
    };
    

#endif