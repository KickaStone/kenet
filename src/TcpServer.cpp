#include <sys/socket.h>
#include <netinet/in.h>

#include "TcpServer.h"
#include "logger/log.h"

TcpServer::TcpServer(const InetAddress& addr)  {
    loop_ = std::make_shared<EventLoop>();
    acceptor_ = std::make_shared<Acceptor>(loop_, addr);
    acceptor_->setNewConnCallback(
        [this](const int fd, const InetAddress& peer) {
            this->onNewConn(fd, peer);
        }
    );
}

void TcpServer::start() const {
    acceptor_->Listen();
    loop_->loop();
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::onNewConn(int fd, const InetAddress& peer) {
    totalConnections_.fetch_add(1);
    LOG_INFO("TcpServer::onNewConn: new connection from %s, total connections: %lu", peer.toIpPort().c_str(), totalConnections_.load());
    
    // get server local address
    sockaddr_in localAddr{};
    socklen_t addrLen = sizeof(localAddr);
    getsockname(fd, reinterpret_cast<sockaddr*>(&localAddr), &addrLen);
    InetAddress local(localAddr);

    auto conn = std::make_shared<TcpConnection>(loop_.get(), fd, local, peer); // shared_ptr
    mu.lock();
    conns_.insert({fd, conn});
    mu.unlock();
    conn->setMessageCallback(onMessage_);
    conn->setConnectionCallback(onConn_);
    conn->setCloseCallback([&](const TcpConnection::Ptr &conn) {removeConn(conn);});
    conn->connectEstablished();
}

void TcpServer::removeConn(const TcpConnection::Ptr& conn) {
    loop_->runInLoop([this,conn]() {
        mu.lock();
        conns_.erase(conn->fd());
        totalConnections_.fetch_sub(1);
        mu.unlock();
        LOG_INFO("TcpServer::removeConn: total connections: %lu", totalConnections_.load());
    });
}

void TcpServer::startInThread() {
    if (running_.load()) {
        LOG_WARN("TcpServer::startInThread: server is already running");
        return;
    }
    
    running_.store(true);
    acceptor_->Listen();
    
    // 创建新线程运行EventLoop
    serverThread_ = std::make_unique<std::thread>(&TcpServer::runInThread, this);
    
    LOG_INFO("TcpServer::startInThread: server started in separate thread");
}

void TcpServer::runInThread() {
    LOG_INFO("TcpServer::runInThread: EventLoop started in thread: %lu", 
             std::hash<std::thread::id>{}(std::this_thread::get_id()));
    
    loop_->loop();
    
    LOG_INFO("TcpServer::runInThread: EventLoop ended");
}

void TcpServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    LOG_INFO("TcpServer::stop: stopping server...");
    running_.store(false);
    
    // 停止EventLoop
    loop_->quit();
    
    // 等待线程结束
    if (serverThread_ && serverThread_->joinable()) {
        serverThread_->join();
    }
    
    LOG_INFO("TcpServer::stop: server stopped");
}

void TcpServer::join() {
    if (serverThread_ && serverThread_->joinable()) {
        serverThread_->join();
    }
}