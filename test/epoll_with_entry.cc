#include <sys/epoll.h>      // epoll相关函数和数据结构
#include <sys/socket.h>     // socket相关函数
#include <netinet/in.h>     // 网络地址结构体
#include <arpa/inet.h>      // 网络地址转换函数
#include <fcntl.h>          // 文件控制选项
#include <unistd.h>         // 系统调用函数
#include <cstdio>           // 标准输入输出
#include <cstring>          // 字符串处理
#include <cerrno>           // 错误码
#include <vector>           // 动态数组

/**
 * 设置文件描述符为非阻塞模式
 * @param fd 要设置的文件描述符
 * @return 成功返回true，失败返回false
 */
static bool setNonBlock(int fd) {
    // F_GETFL: 获取文件描述符的标志位
    int fl = fcntl(fd, F_GETFL, 0);
    if(fl == -1) return false;
    // F_SETFL: 设置文件描述符的标志位，添加O_NONBLOCK非阻塞标志
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK) != -1;
}

// 事件回调
struct Entry {
    int fd; // 文件描述符
    uint32_t events; // 事件类型
    // 四类回调
    void (*onRead)(int);
    void (*onWrite)(int);
    void (*onClose)(int);
    void (*onError)(int);
};

int main() {
    // 创建监听socket
    // AF_INET: IPv4协议族
    // SOCK_STREAM: TCP流套接字
    // SOCK_CLOEXEC: 在执行exec时自动关闭文件描述符
    int lfd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (lfd <0 ) {perror("socket"); return 1;}
     
    // 设置socket选项，允许地址重用
    // SOL_SOCKET: 套接字级别选项
    // SO_REUSEADDR: 允许重用本地地址
    int yes = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    // 设置监听socket为非阻塞模式
    if(!setNonBlock(lfd)) {perror("fcntl O_NONBLOCK"); return 1;}

    // 配置服务器地址结构
    sockaddr_in addr{}; 
    addr.sin_family = AF_INET;                    // IPv4协议族
    addr.sin_port = htons(9000);                  // 端口号9000，转换为网络字节序
    addr.sin_addr.s_addr = htonl(INADDR_ANY);     // 监听所有网络接口，转换为网络字节序
    
    // 绑定socket到指定地址和端口
    if(bind(lfd, (sockaddr*)&addr, sizeof(addr)) < 0) {perror("bind"); return 1;}
    
    // 开始监听连接
    // SOMAXCONN: 系统允许的最大连接队列长度
    if(listen(lfd, SOMAXCONN) < 0) {perror("listen"); return 1;}
    
    // 创建epoll实例
    // EPOLL_CLOEXEC: 在执行exec时自动关闭epoll文件描述符
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if(epfd < 0) {perror("epoll_create1"); return 1;}

    // 配置epoll事件结构
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;    // EPOLLIN: 可读事件，EPOLLET: 边缘触发模式
    ev.data.fd = lfd;                 // 关联的文件描述符
    
    // 将监听socket添加到epoll监控列表
    // EPOLL_CTL_ADD: 添加监控的文件描述符
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) < 0) {perror("epoll_ctl ADD listen"); return 1;}

    printf("Listening on 0.0.0.0:9000\n");
    
    // 用于接收epoll事件的数组，最多64个事件
    std::vector<epoll_event> events(64);

    // 主事件循环
    for(;;){
        // 等待epoll事件
        // -1: 无限等待，直到有事件发生
        int n = epoll_wait(epfd, events.data(), (int)events.size(), -1);
        if(n < 0) {
            if (errno == EINTR) continue;  // 被信号中断，继续等待
            perror("epoll_wait");
            break;
        }
        
        // 处理所有就绪的事件
        for(int i = 0; i <n ; ++i){
            uint32_t re = events[i].events;    // 事件类型
            int fd = events[i].data.fd;        // 发生事件的文件描述符

            // 处理监听socket的可读事件（新连接到达）
            if(fd == lfd && re & EPOLLIN) {
                // 边缘触发模式：必须循环接受所有连接，直到没有更多连接
                for(;;) {
                    sockaddr_in peer{}; 
                    socklen_t len = sizeof(peer);
                    
                    // 接受新连接
                    // SOCK_NONBLOCK: 新连接socket为非阻塞模式
                    // SOCK_CLOEXEC: 在执行exec时自动关闭
                    int cfd = ::accept4(lfd, (sockaddr*)&peer, &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
                    if(cfd > 0){
                        // 打印客户端连接信息
                        char ip[64]; 
                        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));  // 网络地址转字符串
                        printf("Accepted connection from %s:%d\n", ip, ntohs(peer.sin_port));
                        // 立即关闭连接（演示用，实际应用中应该处理连接）
                        close(cfd);
                    } else {
                        // EAGAIN/EWOULDBLOCK: 没有更多连接可接受
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("accept"); break;
                    }
                }
            }

            // 处理错误事件
            if(re & (EPOLLHUP | EPOLLERR)) {
                // EPOLLHUP: 连接挂起
                // EPOLLERR: 连接错误
                int soerr = 0; 
                socklen_t len = sizeof(soerr);
                // 获取socket错误状态
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len);
                printf("epoll event error on fd %d: %s\n", fd, strerror(soerr));
                return 0;
            }
        }
    }
    
    // 清理资源
    close(epfd);  // 关闭epoll实例
    close(lfd);   // 关闭监听socket
    return 0;
}

