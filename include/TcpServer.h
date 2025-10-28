#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <thread>
#include <atomic>


class TcpServer {
    public:
        explicit TcpServer(const InetAddress& addr);
        ~TcpServer();
        void setMessageCallback(TcpConnection::MessageCb cb){ onMessage_ = std::move(cb); }
        void setConnectionCallback(TcpConnection::EventCb cb){ onConn_ = std::move(cb); }
        
        // 阻塞式启动（保持向后兼容）
        void start() const;
        
        // 非阻塞式启动 - 在独立线程中运行EventLoop
        void startInThread();
        
        // 停止服务器
        void stop();
        
        // 等待服务器线程结束
        void join();
    
    private:
        void onNewConn(int fd, const InetAddress& peer);
        void removeConn(const TcpConnection::Ptr& c);
        void runInThread();  // 在线程中运行的函数
    
        std::shared_ptr<EventLoop> loop_;
        std::shared_ptr<Acceptor> acceptor_;
        std::unordered_map<int, TcpConnection::Ptr> conns_;
        std::mutex mu;
        TcpConnection::MessageCb onMessage_;
        TcpConnection::EventCb   onConn_;
        
        // 线程管理
        std::unique_ptr<std::thread> serverThread_;
        std::atomic<bool> running_{false};
        std::atomic<uint64_t> totalConnections_{0};
    };
    

#endif