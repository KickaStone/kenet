#ifndef _POLLER_H_
#define _POLLER_H_

#include <memory>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>

class Channel;

/**
 * @brief Poller是epoll创建的文件描述符的封装，负责监听文件描述符上的事件。
 * 
 */
class Poller {
public:
    explicit Poller();

    ~Poller();

    void updateChannel(const std::shared_ptr<Channel>& ch) const; // ADD/MOD channel
    void removeChannel(const std::shared_ptr<Channel>& ch) const; // DEL channel
    int poll(int timeoutMs, std::vector<Channel*>& active); // poll events
private:
    int epfd_; // epoll fd
    static const int MAX_EVENTS = 1024; // max events to poll
    epoll_event events_[MAX_EVENTS]{}; // events to poll
};

#endif
