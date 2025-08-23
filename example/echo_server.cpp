#include "EventLoop.h"
#include "TcpServer.h"
#include "InetAddress.h"
#include "logger/log.h"
#include <iostream>
#include <string>

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& addr)
        : server_(loop, addr) {
        server_.setConnectionCallback(
            [this](const TcpConnection::Ptr& conn) {
                LOG_INFO("EchoServer: new connection from %d", conn->fd());
            }
        );
        
        server_.setMessageCallback(
            [this](const TcpConnection::Ptr& conn, std::string_view data) {
                LOG_INFO("EchoServer: received %zu bytes from fd %d", data.size(), conn->fd());
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
    // init logger
    using Logger = AsyncLogger<512, 4096>;
    Config cfg;
    cfg.path = "server.log";
    cfg.file_level = Level::Debug;   // 文件从 Debug 开始记
    cfg.console_level = Level::Warn; // 控制台只打 Warn+
    cfg.flush_bytes = 128<<10;
    cfg.flush_interval = std::chrono::milliseconds(100);
    cfg.console_enable_color = true;

    auto& L = Logger::instance();
    L.start(cfg);



    LOG_INFO("EchoServer starting...");
    
    EventLoop loop;
    InetAddress addr("127.0.0.1", 9000);
    EchoServer server(&loop, addr);

    
    server.start();
    
    LOG_INFO("EchoServer started on %s", addr.toIpPort().c_str());
    LOG_INFO("Press Ctrl+C to stop");
    
    loop.loop();
    
    return 0;
}