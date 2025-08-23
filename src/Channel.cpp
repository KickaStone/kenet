#include "../include/Channel.h"
#include "../include/EventLoop.h"
#include "../include/logger/log.h"

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd) {}

void Channel::setReadCallback(Callback cb) {
    readCb_ = cb;
}

void Channel::setWriteCallback(Callback cb) {
    writeCb_ = cb;
}

void Channel::setCloseCallback(Callback cb) {
    closeCb_ = cb;
}

void Channel::setErrorCallback(Callback cb) {
    errorCb_ = cb;
}

void Channel::handleEvent(uint32_t revents) {
    if (revents & EPOLLIN) {
        LOG_INFO("Channel::handleEvent: EPOLLIN on fd: %d", fd_);
        if (readCb_) readCb_();
    }
    if (revents & EPOLLOUT) {
        LOG_INFO("Channel::handleEvent: EPOLLOUT on fd: %d", fd_);
        if (writeCb_) writeCb_();
    }
    if (revents & EPOLLRDHUP) {
        LOG_INFO("Channel::handleEvent: EPOLLRDHUP on fd: %d", fd_);
        if (closeCb_) closeCb_();
    }
    if (revents & EPOLLERR) {
        LOG_INFO("Channel::handleEvent: EPOLLERR on fd: %d", fd_);
        if (errorCb_) errorCb_();
    }
}


void Channel::setEvents(uint32_t events) {
    events_ = events;
}

void Channel::update() {
    loop_->updateChannel(this);
}
    
