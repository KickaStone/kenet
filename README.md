# Kenet - 基于Reactor模式的C++网络库

这是一个基于Reactor模式的C++网络库，实现了事件驱动的网络编程模型。

## 架构设计

### 核心组件

- **EventLoop**: 事件循环，每个I/O线程一个循环，封装epoll
- **Poller**: 对epoll的封装，负责事件监听
- **Channel**: FD与感兴趣事件的映射，处理回调
- **Acceptor**: 接受新连接的组件
- **TcpConnection**: TCP连接的封装
- **TcpServer**: TCP服务器的封装

### 设计模式

- **Reactor模式**: 事件驱动，非阻塞I/O
- **单线程模型**: 每个EventLoop运行在独立线程中
- **回调机制**: 通过回调函数处理各种事件

## 编译和运行

### 编译

```bash
make clean
make
```

### 运行示例

1. 启动echo服务器:
```bash
./echo_server
```

2. 在另一个终端运行客户端:
```bash
./echo_client
```

## 示例代码

### Echo服务器

```cpp
#include "net/EventLoop.h"
#include "net/TcpServer.h"
#include "net/InetAddress.h"

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& addr)
        : server_(loop, addr) {
        server_.setMessageCallback(
            [this](const TcpConnection::Ptr& conn, std::string_view data) {
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
    EventLoop loop;
    InetAddress addr("127.0.0.1", 9000);
    EchoServer server(&loop, addr);
    
    server.start();
    loop.loop();
    
    return 0;
}
```

## 特性

- ✅ 基于epoll的事件驱动
- ✅ 非阻塞I/O
- ✅ 线程安全的事件循环
- ✅ 自动内存管理
- ✅ 优雅的错误处理
- ✅ 完整的echo服务器示例

## 文件结构

```
net/
├── EventLoop.h/cpp      # 事件循环
├── Poller.h/cpp         # epoll封装
├── Channel.h/cpp        # 事件通道
├── Acceptor.h/cpp       # 连接接受器
├── TcpConnection.h/cpp  # TCP连接
├── TcpServer.h/cpp      # TCP服务器
├── InetAddress.h/cpp    # 网络地址
└── Logger.h/cpp         # 日志系统

example/
├── echo_server.cpp      # Echo服务器示例
└── echo_client.cpp      # Echo客户端示例
```

## 依赖

- Linux系统
- C++17编译器
- pthread库

## 许可证

MIT License