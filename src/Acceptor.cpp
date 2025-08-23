#include "Acceptor.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "logger/log.h"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr) : loop_(loop), addr_(addr) {
    listenfd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listenfd_ < 0) {
        LOG_FATAL("socket failed");
        exit(1);
    }
    channel_ = std::make_unique<Channel>(loop_, listenfd_);
    channel_->setReadCallback(std::bind(&Acceptor::handleAccept, this));
}

void Acceptor::Listen() {
    // set SO_REUSEADDR
    int opt = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    auto addr = addr_.getSockAddr();
    int ret = bind(listenfd_, (const struct sockaddr*)addr, sizeof(*addr));
    if (ret < 0) {
        LOG_FATAL("bind failed: %s (errno: %d)", strerror(errno), errno);
        exit(1);
    }
    channel_->enableReading();

    ret = listen(listenfd_, 1024);
    if (ret < 0) {
        LOG_FATAL("listen failed: %s (errno: %d)", strerror(errno), errno);
        exit(1);
    }
}

void Acceptor::handleAccept() {
    if (channel_->revents() & EPOLLIN) {
        sockaddr_in peerAddr;
        socklen_t addrLen = sizeof(peerAddr);

        while (true) {
            int connfd = accept4(listenfd_, (struct sockaddr*)&peerAddr, &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (connfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // no more connections to accept
                    break;
                }
                LOG_ERROR("accept4 failed: %s", strerror(errno));
                break;
            }

            if (cb_) {
                InetAddress peer(peerAddr);
                cb_(connfd, peer);
            } else {
                close(connfd);
            }
        }
    }
}