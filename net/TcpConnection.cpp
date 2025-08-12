#include "TcpConnection.h"


TcpConnection::TcpConnection(EventLoop* loop, int fd, InetAddress local, InetAddress peer) : loop_(loop), fd_(fd), local_(local), peer_(peer) {
    channel_ = std::make_unique<Channel>(loop_, fd);
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection() {
    channel_->disableAll();
}

void TcpConnection::connectEstablished() {
    state_ = kConnected;
    channel_->enableReading();
    if (onConn_) {
        onConn_(shared_from_this());
    }
}

void TcpConnection::send(std::string_view data) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(data);
        } else {
            loop_->runInLoop([this, data = std::string(data)]() {
                sendInLoop(data);
            });
        }
    }
}

void TcpConnection::sendInLoop(std::string_view data) {
    if (outBuf_.empty()) {
        // 直接尝试写入
        ssize_t n = write(fd_, data.data(), data.size());
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区满，添加到输出缓冲区
                outBuf_.append(data.data(), data.size());
                channel_->enableWriting();
            } else {
                Logger::Error("TcpConnection::sendInLoop: write failed: %s", strerror(errno));
            }
        } else if (static_cast<size_t>(n) < data.size()) {
            // 部分写入，剩余部分添加到输出缓冲区
            outBuf_.append(data.data() + n, data.size() - n);
            channel_->enableWriting();
        }
    } else {
        // 输出缓冲区非空，直接添加到缓冲区
        outBuf_.append(data.data(), data.size());
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        loop_->runInLoop([this]() {
            channel_->disableWriting();
        });
    }
}

void TcpConnection::handleRead() {
    if (channel_->revents() & EPOLLIN) {
        char buf[4096];
        ssize_t n = read(fd_, buf, sizeof(buf));
        Logger::Info("TcpConnection::handleRead: fd=%d, read=%zd", fd_, n);
        if (n > 0) {
            if (onMessage_) {
                Logger::Info("TcpConnection::handleRead: calling onMessage_ with %zu bytes", n);
                onMessage_(shared_from_this(), std::string_view(buf, n));
            } else {
                Logger::Info("TcpConnection::handleRead: onMessage_ is null");
            }
        } else if (n == 0) {
            Logger::Info("TcpConnection::handleRead: connection closed by peer");
            handleClose();
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                Logger::Error("TcpConnection::handleRead: read failed: %s", strerror(errno));
                handleError();
            } else {
                Logger::Info("TcpConnection::handleRead: EAGAIN/EWOULDBLOCK");
            }
        }
    }
}

void TcpConnection::handleWrite() {
    if (channel_->revents() & EPOLLOUT) {    
        ssize_t n = write(fd_, outBuf_.data(), outBuf_.size());
        if (n > 0) {
            outBuf_.erase(0, n);
            if (outBuf_.empty()) {
                channel_->disableWriting();
                if (onWriteComplete_) {
                    onWriteComplete_(shared_from_this());
                }
            }
        } else if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                Logger::Error("TcpConnection::handleWrite: write failed: %s", strerror(errno));
            }
        }
    }
}

void TcpConnection::handleClose() {
    state_ = kDisconnected;
    channel_->disableAll();
    if (onClose_) {
        onClose_(shared_from_this());
    }
}

// 处理错误
void TcpConnection::handleError() {
    // erron 其实不可靠，epollerr事件触发时，errno可能已经被其他系统调用修改，不再反映真实原因
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        Logger::Error("TcpConnection::handleError: getsockopt failed: %s", strerror(errno));
    } else {
        Logger::Error("TcpConnection::handleError: socket error: %s", strerror(error));
    }
    state_ = kDisconnected;
    channel_->disableAll();
    if (onClose_) {
        onClose_(shared_from_this());
    }
}