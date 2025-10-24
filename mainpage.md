# Kenet - 基于Reactor模式的C++网络库

## 项目简介

Kenet是一个基于Reactor模式的C++网络库，实现了事件驱动的网络编程模型。该库提供了高性能、易用的网络编程接口，支持TCP服务器和客户端的开发。

## 核心特性

- ✅ **基于epoll的事件驱动**: 使用Linux epoll机制实现高效的I/O多路复用
- ✅ **非阻塞I/O**: 全异步非阻塞网络编程模型
- ✅ **线程安全**: 每个EventLoop运行在独立线程中，保证线程安全
- ✅ **自动内存管理**: 智能指针管理连接生命周期
- ✅ **优雅的错误处理**: 完善的异常处理和错误恢复机制
- ✅ **简洁的API**: 易于使用的接口设计

## 架构设计

### 核心组件

Kenet网络库采用Reactor模式，主要包含以下核心组件：

#### EventLoop
事件循环的核心，每个I/O线程运行一个EventLoop实例。负责：
- 管理事件循环的生命周期
- 处理定时器事件
- 协调各个组件的协作

#### Poller
对Linux epoll的封装，负责：
- 监听文件描述符上的事件
- 向EventLoop报告就绪的事件
- 管理epoll实例

#### Channel
文件描述符与感兴趣事件的映射，负责：
- 封装文件描述符和事件类型
- 处理各种事件的回调函数
- 管理事件的状态

#### Acceptor
接受新连接的组件，负责：
- 监听服务器套接字
- 接受新的客户端连接
- 创建TcpConnection对象

#### TcpConnection
TCP连接的封装，负责：
- 管理单个TCP连接的生命周期
- 处理数据的发送和接收
- 提供连接状态管理

#### TcpServer
TCP服务器的封装，负责：
- 管理多个TcpConnection
- 提供服务器级别的接口
- 协调Acceptor和连接管理

### 设计模式

- **Reactor模式**: 事件驱动，非阻塞I/O
- **单线程模型**: 每个EventLoop运行在独立线程中
- **回调机制**: 通过回调函数处理各种事件

## 快速开始

### 编译项目

```bash
mkdir build && cd build
cmake ..
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

## 使用示例

### Echo服务器

```cpp
#include "kenet.h"

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

## 项目结构

```
kenet/
├── include/                 # 头文件目录
│   ├── EventLoop.h         # 事件循环
│   ├── Poller.h            # epoll封装
│   ├── Channel.h           # 事件通道
│   ├── Acceptor.h          # 连接接受器
│   ├── TcpConnection.h     # TCP连接
│   ├── TcpServer.h         # TCP服务器
│   ├── InetAddress.h       # 网络地址
│   ├── Socket.h            # 套接字封装
│   └── logger/             # 日志系统
├── src/                    # 源文件目录
│   ├── EventLoop.cpp
│   ├── Poller.cpp
│   ├── Channel.cpp
│   ├── Acceptor.cpp
│   ├── TcpConnection.cpp
│   ├── TcpServer.cpp
│   └── InetAddress.cpp
├── example/                # 示例代码
│   ├── echo_server.cpp     # Echo服务器示例
│   └── echo_client.cpp     # Echo客户端示例
├── test/                   # 测试代码
└── docs/                   # 生成的文档
```

## 依赖要求

- **操作系统**: Linux
- **编译器**: 支持C++17的编译器 (GCC 7+ 或 Clang 5+)
- **依赖库**: pthread
- **构建工具**: CMake 3.14+

## 许可证

MIT License

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 联系方式

如有问题或建议，请通过GitHub Issues联系我们。
