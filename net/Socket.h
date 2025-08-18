#ifndef _NET_SOCKET_H_
#define _NET_SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include "InetAddress.h"

// set non-block
inline void setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); // copy flag 
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // set flag
}

// set no delay
// do not use Nagle algorithm
inline void setNoDelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

// create listen_fd
inline int createListenFd(const InetAddress& addr) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        Logger::Fatal("create listen_fd failed");
        return -1;
    }
    // sockin to addr
    const sockaddr_in *addr_ = addr.getSockAddr();
    bind(listen_fd, (sockaddr*)addr_, sizeof(*addr_));
    if (listen_fd == -1) {
        Logger::Info("create listen_fd failed");
        return -1;
    }
    return listen_fd;
}


#endif