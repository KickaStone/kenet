# Async Logger

基于SPSC（单生产者单消费者） ring buffer

## 优化点

### 1. 锁竞争和上下文切换
- 避免多线程竞争同一锁，不使用全局mutex而是使用SPSC无锁队列，每个线程拥有环形队列；
- 写端只有本线程，读端是日志线程；无锁，仅原子索引
- 没有CAS/自旋/阻塞


### 2. 优化系统调用
- `write()`和`fwrite()`每次调用都要进入内核，频繁写日志syscall开销很大
- 使用日志聚合 + 阈值触发，批量写入日志

### 3. 内存分配与拷贝
- ringbuffer 

### 4. 时间戳
- 优化 `gettimeofday` `clock_gettime` 与复杂 strftime 字符串格式化
- 使用粗粒度时钟`CLOCK_REALTIME_COARSE`，这是Linux系统中获取时钟的另一个方式，对比`CLOCK_REALTIME`精度低但速度快

## Benchmark

10轮写入100000条日志

```shell
Round 1: 157.297 ms to log 1000000 messages.
Round 2: 107.746 ms to log 1000000 messages.
Round 3: 106.12 ms to log 1000000 messages.
Round 4: 84.7132 ms to log 1000000 messages.
Round 5: 83.587 ms to log 1000000 messages.
Round 6: 86.1602 ms to log 1000000 messages.
Round 7: 84.5947 ms to log 1000000 messages.
Round 8: 83.1248 ms to log 1000000 messages.
Round 9: 122.263 ms to log 1000000 messages.
Round 10: 83.9588 ms to log 1000000 messages.
```

