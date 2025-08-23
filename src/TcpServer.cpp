#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "TcpServer.h"
#include "logger/log.h"

TcpServer::TcpServer(EventLoop* loop, const InetAddress& addr) : loop_(loop), acceptor_(loop, addr) {
    acceptor_.setNewConnCallback(
        [this](int fd, const InetAddress& peer) {
            this->onNewConn(fd, peer);
        }
    );
}

void TcpServer::start() {
    acceptor_.Listen();
}

void TcpServer::onNewConn(int fd, const InetAddress& peer) {
    LOG_INFO("TcpServer::onNewConn: new connection from %s", peer.toIpPort().c_str());
    
    // get server local address
    sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    getsockname(fd, (struct sockaddr*)&localAddr, &addrLen);
    InetAddress local(localAddr);
    
    auto conn = std::make_shared<TcpConnection>(loop_, fd, local, peer); // shared_ptr
    conns_.insert({fd, conn});
    conn->setMessageCallback(onMessage_);
    conn->setConnectionCallback(onConn_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConn, this, conn));
    conn->connectEstablished();
}

void TcpServer::removeConn(const TcpConnection::Ptr& conn) {
    loop_->runInLoop([this, conn]() {
        this->conns_.erase(conn->fd());
    });
}