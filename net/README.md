# 架构

## NetCore（Reactor 子系统）

- EventLoop：每个 I/O 线程一个循环，封装 epoll/kqueue；负责事件分发、定时器轮询、跨线程任务队列（eventfd）。
- Poller：对 epoll 的薄封装（ET/oneshot 可选）。
- Channel：FD 与感兴趣事件（读/写/错误/关闭）的映射，回调在 EventLoop 线程执行。
- TimerQueue：分层时间轮或小根堆（连接空闲、ACK 超时、应用超时）。
- Buffer：IOBuffer（readv/writev 友好，支持零拷贝片段）+ FlatBuffer（短消息）。支持水位线、scatter/gather。


### Channel 是什么