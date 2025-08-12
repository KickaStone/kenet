# 架构

## NetCore（Reactor 子系统）

- EventLoop：每个 I/O 线程一个循环，封装 epoll/kqueue；负责事件分发、定时器轮询、跨线程任务队列（eventfd）。
- Poller：对 epoll 的薄封装（ET/oneshot 可选）。
- Channel：FD 与感兴趣事件（读/写/错误/关闭）的映射，回调在 EventLoop 线程执行。
- TimerQueue：分层时间轮或小根堆（连接空闲、ACK 超时、应用超时）。
- Buffer：IOBuffer（readv/writev 友好，支持零拷贝片段）+ FlatBuffer（短消息）。支持水位线、scatter/gather。

## 模型
```
┌─────────────────────────────────────────────────────────────┐
│                   Application Layer                         │
├─────────────────────────────────────────────────────────────┤
│  TcpServer                                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │TcpConnection│  │TcpConnection│  │TcpConnection│          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
├─────────────────────────────────────────────────────────────┤
│                   Event Abstraction                         │
├─────────────────────────────────────────────────────────────┤
│  Acceptor      Channel      Channel      Channel            │
│  ┌─────────┐   ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Channel │   │ Event   │  │ Event   │  │ Event   │        │
│  └─────────┘   │ Loop    │  │ Loop    │  │ Loop    │        │
│                └─────────┘  └─────────┘  └─────────┘        │
├─────────────────────────────────────────────────────────────┤
│                    Event Loop                               │
├─────────────────────────────────────────────────────────────┤
│                    EventLoop                                │
│                ┌─────────────────┐                          │
│                │     Poller      │                          │
│                │  ┌───────────┐  │                          │
│                │  │   epoll   │  │                          │
│                │  └───────────┘  │                          │
│                └─────────────────┘                          │
├─────────────────────────────────────────────────────────────┤
│                    系统调用层 (System Calls)                 │
├─────────────────────────────────────────────────────────────┤
│  Socket API    InetAddress    Logger                        │
└─────────────────────────────────────────────────────────────┘
```

## 处理流程

### 服务器启动
```
应用程序启动
    ↓
EventLoop 创建
    ↓
TcpServer 创建 (包含 Acceptor)
    ↓
TcpServer::start() 调用
    ↓
Acceptor::Listen() 执行
    ├── socket() 创建监听套接字 (非阻塞)
    ├── setsockopt(SO_REUSEADDR) 设置地址重用
    ├── bind() 绑定地址和端口
    ├── listen() 开始监听
    └── channel_->enableReading() 注册到 epoll
    ↓
EventLoop::loop() 启动事件循环
    ↓
epoll_wait() 等待事件
```

### 连接建立

```
客户端发起连接请求
    ↓
epoll 检测到监听套接字可读事件
    ↓
EventLoop::loop() 调用 poller_.poll()
    ↓
Poller::poll() 返回活跃的 Channel
    ↓
Channel::handleEvent() 处理事件
    ↓
Acceptor::handleRead() 被调用
    ↓
accept4() 接受新连接 (非阻塞)
    ↓
创建 InetAddress 对象 (客户端地址)
    ↓
调用 Acceptor 的回调函数 cb_(connfd, peer)
    ↓
TcpServer::onNewConn() 被调用
    ├── 获取本地地址 (getsockname)
    ├── 创建 TcpConnection 对象
    ├── 设置各种回调函数
    ├── 添加到连接映射表 conns_
    └── 调用 connectEstablished()
        ↓
        TcpConnection::connectEstablished()
        ├── 设置状态为 kConnected
        ├── channel_->enableReading() 注册读事件
        └── 调用连接建立回调 onConn_
```

### 数据读写

```
客户端发送数据
    ↓
epoll 检测到连接套接字可读事件
    ↓
EventLoop::loop() 分发事件
    ↓
TcpConnection::handleRead() 被调用
    ↓
read() 读取数据
    ├── 读取成功 (n > 0)
    │   └── 调用 onMessage_ 回调处理数据
    ├── 连接关闭 (n == 0)
    │   └── 调用 handleClose()
    └── 读取失败 (n < 0)
        ├── EAGAIN/EWOULDBLOCK: 继续等待
        └── 其他错误: 调用 handleError()


应用程序调用 TcpConnection::send()
    ↓
检查是否在 EventLoop 线程中
    ├── 是: 直接调用 sendInLoop()
    └── 否: 通过 runInLoop() 排队执行
        ↓
        TcpConnection::sendInLoop()
        ├── 检查输出缓冲区是否为空
        │   ├── 空: 直接调用 write()
        │   │   ├── 写入成功: 完成
        │   │   ├── 部分写入: 剩余数据加入缓冲区
        │   │   └── 缓冲区满: 数据加入缓冲区，启用写事件
        │   └── 非空: 数据直接加入缓冲区
        └── 等待 epoll 写事件
            ↓
            TcpConnection::handleWrite()
            ├── write() 写入缓冲区数据
            ├── 更新缓冲区状态
            ├── 缓冲区空: 禁用写事件
            └── 调用 onWriteComplete_ 回调
```

### 连接关闭

```
连接关闭触发 (客户端断开/错误/主动关闭)
    ↓
epoll 检测到关闭事件 (EPOLLRDHUP/EPOLLERR)
    ↓
TcpConnection::handleClose() 或 handleError()
    ├── 设置状态为 kDisconnected
    ├── channel_->disableAll() 取消所有事件监听
    └── 调用 onClose_ 回调
        ↓
        TcpServer::removeConn() 被调用
        ├── 从连接映射表 conns_ 中移除
        └── 连接对象自动析构
            ↓
            TcpConnection::~TcpConnection()
            └── channel_->disableAll() 确保清理
```

## TODO

- [ ] 优雅关闭
- [ ] 多线程模型
- [ ] 线程池
- [ ] 异步日志

**高级特性**

1. 监控
2. 协议拔插
3. 易用性API
4. 连接池
5. 多路复用