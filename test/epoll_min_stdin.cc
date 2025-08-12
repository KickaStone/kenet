#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <vector>
#include <sys/socket.h>

static bool setNonBlock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if(fl == -1) return false;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK) != -1;
}

int main() {
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if(epfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    int fd = STDIN_FILENO;
    if(!setNonBlock(fd)) {
        perror("fcntl O_NONBLOCK");
        return 1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl ADD stdin"); return 1;
    }

    printf("Type something then Enter (Ctrl+D to send EOF)...\n");

    std::vector<epoll_event> events(16);
    char buf[65536];

    while(true) {
        int n = epoll_wait(epfd, events.data(), (int)events.size(), -1);
        if(n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        for(int i = 0; i <n ; ++i){
            uint32_t re = events[i].events;
            int evfd = events[i].data.fd;

            if(re & (EPOLLHUP | EPOLLERR)) {
                int soerr = 0; socklen_t len = sizeof(soerr);
                getsockopt(evfd, SOL_SOCKET, SO_ERROR, &soerr, &len);
                printf("epoll event error on fd %d: %s\n", evfd, strerror(soerr));
                return 0;
            }

            if(re & EPOLLIN) {
                // ET
                for(;;) {
                    ssize_t m = ::read(evfd, buf, sizeof(buf));
                    if(m > 0) {
                        printf("[READ] %zd bytes: \"", m);
                        fwrite(buf, 1, m, stdout);
                        printf("\"\n");
                    } else if(m == 0) {
                        printf("[EOF] stdin EOF\n");
                        close(epfd);
                        return 0;
                    } else {
                        if(errno == EINTR) continue;
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        perror("read");
                        close(epfd);
                        return 1;
                    }
                }
            }
        }
    }
}