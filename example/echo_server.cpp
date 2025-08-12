#include "net/EventLoop.h"
#include "net/TcpServer.h"
#include "net/InetAddress.h"
#include "net/Logger.h"
#include <iostream>
#include <string>

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& addr)
        : server_(loop, addr) {
        server_.setConnectionCallback(
            [this](const TcpConnection::Ptr& conn) {
                Logger::Info("EchoServer: new connection from %d", conn->fd());
            }
        );
        
        server_.setMessageCallback(
            [this](const TcpConnection::Ptr& conn, std::string_view data) {
                Logger::Info("EchoServer: received %zu bytes from fd %d", data.size(), conn->fd());
                // Echo back the received data
                conn->send(data);
            }
        );
    }
    
    void start() {
        server_.start();
    }
    
private:
    TcpServer server_;
};

int main() {
    Logger::Info("EchoServer starting...");
    
    EventLoop loop;
    InetAddress addr("127.0.0.1", 9000);
    EchoServer server(&loop, addr);
    
    server.start();
    
    Logger::Info("EchoServer started on %s", addr.toIpPort().c_str());
    Logger::Info("Press Ctrl+C to stop");
    
    loop.loop();
    
    return 0;
}