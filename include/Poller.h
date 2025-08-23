#ifndef _NET_POLLER_H_
#define _NET_POLLER_H_

#include <vector>
#include <sys/epoll.h>
#include <unistd.h>

class Channel;

class Poller
{
public:
    explicit Poller();
    ~Poller();
    void updateChannel(Channel *ch);                         // ADD/MOD channel
    void removeChannel(Channel *ch);                         // DEL channel
    int poll(int timeoutMs, std::vector<Channel *> &active); // poll events
private:
    int epfd_;
    static const int MAX_EVENTS = 1024;        // max events to poll
    epoll_event events_[MAX_EVENTS];           // events to poll
};

#endif