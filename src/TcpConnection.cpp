#include "TcpConnection.h"
#include "logger/log.h"


TcpConnection::TcpConnection(EventLoop* loop, int fd, InetAddress local, InetAddress peer) : loop_(loop), fd_(fd), local_(local), peer_(peer) {
    channel_ = std::make_shared<Channel>(&loop_->poller(), fd);
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection() {
    channel_->disableAll();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - establishTime_).count();
    LOG_DEBUG("TcpConnection::~TcpConnection: fd=%d, duration=%ldms", fd_, durationMs);
}

void TcpConnection::connectEstablished() {
    state_ = kConnected;
    channel_->tie(shared_from_this());  // 在对象创建后设置 tie
    channel_->enableReading();
    LOG_INFO("TcpConnection::connectEstablished: fd=%d, state=%d", fd_, state_);
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
                LOG_ERROR("TcpConnection::sendInLoop: write failed: %s", strerror(errno));
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
        LOG_INFO("TcpConnection::handleRead: fd=%d, read=%zd", fd_, n);
        if (n > 0) {
            if (onMessage_) {
                LOG_INFO("TcpConnection::handleRead: calling onMessage_ with %zu bytes", n);
                onMessage_(shared_from_this(), std::string_view(buf, n));
            } else {
                LOG_INFO("TcpConnection::handleRead: onMessage_ is null");
            }
        } else if (n == 0) {
            LOG_INFO("TcpConnection::handleRead: connection closed by peer");
            handleClose();
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::handleRead: read failed: %s", strerror(errno));
                handleError();
            } else {
                LOG_INFO("TcpConnection::handleRead: EAGAIN/EWOULDBLOCK"); // no data to read
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
                LOG_ERROR("TcpConnection::handleWrite: write failed: %s", strerror(errno));
            }
        }
    }
}

void TcpConnection::handleClose() {
    LOG_INFO("TcpConnection::handleClose: fd=%d, state=%d", fd_, state_);
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
        LOG_ERROR("TcpConnection::handleError: getsockopt failed: %s", strerror(errno));
    } else {
        LOG_ERROR("TcpConnection::handleError: socket error: %s", strerror(error));
    }
    state_ = kDisconnected;
    channel_->disableAll();
    if (onClose_) {
        onClose_(shared_from_this());
    }
}