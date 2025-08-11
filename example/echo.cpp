#include "net/EventLoop.h"
#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include "net/InetAddress.h"


int main() {
    EventLoop loop;
    InetAddress addr("0.0.0.0", 9000);
    TcpServer svr(&loop, addr);

    svr.setConnectionCallback([](const TcpConnection::Ptr& c){
        printf("conn %d %s\n", c->fd(), "connected");
    });
    svr.setMessageCallback([](const TcpConnection::Ptr& c, std::string_view msg){
        // 仅示例：同步回发（在 loop 线程）
        c->send(msg);
    });

    svr.start();
    loop.loop();
}