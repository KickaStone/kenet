#include "EventLoop.h"
#include "TcpServer.h"
#include "InetAddress.h"
#include "logger/log.h"
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>




class EchoServer {
public:
    EchoServer(const InetAddress& addr)
        : server_(addr) {
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

    ~EchoServer() {
        LOG_INFO("Echo server is quiting ..");
    }
    
    void start() {
        // 使用非阻塞启动
        server_.startInThread();
    }
    
    void stop() {
        server_.stop();
    }
    
private:
    TcpServer server_;
};

// 全局变量用于信号处理
std::atomic<bool> g_running{true};
EchoServer* g_server = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    LOG_INFO("Received signal %d, shutting down gracefully...", signal);
    g_running.store(false);
    if (g_server) {
        g_server->stop();
    }
}
int main() {
    // init logger
    using Logger = AsyncLogger<512, 4096>;
    Config cfg;
    cfg.path = "server.log";
    cfg.file_level = Level::Debug;   // file log from Debug
    cfg.console_level = Level::Info; // console log from Info+
    cfg.flush_bytes = 128<<10;
    cfg.flush_interval = std::chrono::milliseconds(100);
    cfg.console_enable_color = true;

    auto& L = Logger::instance();
    L.start(cfg);

    LOG_INFO("EchoServer starting...");

    InetAddress addr("127.0.0.1", 9000);
    EchoServer server(addr);
    
    // 设置全局服务器指针用于信号处理
    g_server = &server;
    
    // 注册信号处理函数
    std::signal(SIGINT, signalHandler);   // Ctrl+C
    std::signal(SIGTERM, signalHandler);  // 终止信号
    
    // 非阻塞启动服务器
    server.start();
    
    LOG_INFO("EchoServer started on %s", addr.toIpPort().c_str());
    LOG_INFO("Press Ctrl+C to stop");
    
    // 主循环 - 等待信号
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("EchoServer shutting down...");
    
    // 清理资源
    g_server = nullptr;
    
    LOG_INFO("EchoServer stopped");
    return 0;
}