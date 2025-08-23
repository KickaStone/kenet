#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_

#include <string>
#include <string_view>
#include <memory>
#include <functional>

#include "InetAddress.h"
#include "EventLoop.h"
#include "Channel.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    using MessageCb = std::function<void(const Ptr &, std::string_view)>;
    using EventCb = std::function<void(const Ptr &)>;

    TcpConnection(EventLoop *loop, int fd, InetAddress local, InetAddress peer);
    ~TcpConnection();

    void setMessageCallback(MessageCb cb) { onMessage_ = std::move(cb); }
    void setConnectionCallback(EventCb cb) { onConn_ = std::move(cb); }
    void setCloseCallback(EventCb cb) { onClose_ = std::move(cb); }
    void setWriteCompleteCallback(EventCb cb) { onWriteComplete_ = std::move(cb); }

    void connectEstablished();        // 由 TcpServer 在所属 loop 中调用：install 回调，enableReading
    void send(std::string_view data); // 仅在所属 loop 线程调用（MVP）
    void shutdown();                  // 优雅关闭（仅 loop 线程调用）
    int fd() const { return fd_; }

private:
    void sendInLoop(std::string_view data);
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    EventLoop *loop_;
    const int fd_;
    std::unique_ptr<Channel> channel_;
    enum State
    {
        kConnecting,
        kConnected,
        kDisconnecting,
        kDisconnected
    } state_{kConnecting};
    std::string inBuf_, outBuf_;
    InetAddress local_, peer_;

    MessageCb onMessage_;
    EventCb onConn_, onClose_, onWriteComplete_;
};

#endif