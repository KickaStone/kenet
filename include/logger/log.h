#ifndef LOG_H
#define LOG_H
#include <chrono>
#include <condition_variable>
#include <string>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <vector>

#include "sink.h"
#include "ring_buffer.h"


// Log Record
template<size_t MaxLine>
struct LogEntry {
    uint64_t ts_ns; // CLOCK_REALTIME_*(coarse)
    uint32_t tid; // hashed thread id
    Level level; // log level
    uint16_t len; // bytes used in msg
    const char* file; // trace filename
    const char* func; // trace function
    int line; // trace line
    char msg[MaxLine]; //payload(formatted by producer)
};


struct Config {
    // 文件
    std::string path = "app.log";
    size_t rotate_bytes = 128ull << 20;
    size_t flush_bytes  = 64ull  << 10;
    std::chrono::milliseconds flush_interval{200};
    bool fsync_on_flush = false;

    // 等级控制（关键）
    Level file_level    = Level::Info;
    Level console_level = Level::Warn;

    // console
    bool console_enable_color = true;
    
    // 默认配置：只启用控制台输出，level=debug
    static Config default_config() {
        Config cfg;
        cfg.path = "app.log";
        cfg.file_level = Level::Off;      // 禁用文件输出
        cfg.console_level = Level::Debug; // 控制台从Debug开始
        cfg.flush_bytes = 128ull << 10;   // 128KB
        cfg.flush_interval = std::chrono::milliseconds(100);
        cfg.console_enable_color = true;
        return cfg;
    }
};


template <size_t MaxLine = 512, size_t RingCap = 4096>
class AsyncLogger {
    using Entry = LogEntry<MaxLine>;
    using Ring  = SpscRing<Entry, RingCap>;

    struct Producer {
        std::unique_ptr<Ring> ring{new Ring()};
        std::atomic<uint64_t> dropped{0};
    };

public:
    static AsyncLogger& instance() {
        static AsyncLogger g;
        // 自动启动logger（如果还没有启动）
        if (!g.running_) {
            g.start();
            // 注册程序退出时的清理函数
            std::atexit([]() { AsyncLogger::auto_stop(); });
        }
        return g;
    }

    void start(Config cfg = Config::default_config()) {
        std::lock_guard<std::mutex> lk(mu_);
        if (running_) return;
        cfg_ = std::move(cfg);
        // open_file_locked();

        // init sinks
        file_sink_ = std::make_unique<FileSink>(cfg_.path, cfg_.fsync_on_flush, cfg_.rotate_bytes);
        file_sink_->min_level = cfg_.file_level;

        console_sink_ = std::make_unique<ConsoleSink>(cfg_.console_enable_color);
        console_sink_->min_level = cfg_.console_level;

        running_ = true;
        worker_ = std::thread([this]{ run(); });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (!running_) return;
            running_ = false;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
        if (fd_ >= 0) ::close(fd_);
        fd_ = -1;
    }

    ~AsyncLogger() { stop(); }
    
    // 程序退出时自动停止logger
    static void auto_stop() {
        static bool stopped = false;
        if (!stopped) {
            stopped = true;
            instance().stop();
        }
    }

    // 可在任意线程调用
    void logf(Level lv, const char* fmt, ...) {
        auto* prod = get_or_create_producer();
        Entry e{};
        e.level = static_cast<Level>(lv);
        e.tid   = thread_id_hash();
        e.ts_ns = now_ns_coarse();

        char buf[MaxLine];
        va_list ap; va_start(ap, fmt);
        int n = vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        if (n < 0) return;

        if (static_cast<size_t>(n) >= sizeof(buf)) {
            // 截断并标注
            constexpr const char* tail = "...(trunc)";
            size_t keep = sizeof(buf) - 1;
            size_t tl   = strlen(tail);
            if (keep > tl) {
                memcpy(buf + keep - tl, tail, tl);
            }
            n = static_cast<int>(keep);
        }
        e.len = static_cast<uint16_t>(n);
        memcpy(e.msg, buf, e.len);

        if (!prod->ring->try_push(e)) {
            prod->dropped.fetch_add(1, std::memory_order_relaxed);
        } else {
            // 轻量唤醒：避免纯自旋
            if (++wake_hint_ % 1024 == 0) cv_.notify_one();
        }
    }

    void _logf_impl(Level lv, const char* file, int line, const char* func, const char* fmt, ...) {
        auto* prod = get_or_create_producer();
        Entry e{};
        e.level = lv;
        e.tid   = thread_id_hash();
        e.ts_ns = now_ns_coarse();
        e.file  = file;
        e.func  = func;
        e.line  = line;

        char buf[MaxLine];
        va_list ap; va_start(ap, fmt);
        int n = vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

        if (n < 0) return;
        if ((size_t)n >= sizeof(buf)) n = sizeof(buf) - 1;
        e.len = static_cast<uint16_t>(n);
        memcpy(e.msg, buf, e.len);

        if (!prod->ring->try_push(e)) {
            prod->dropped.fetch_add(1, std::memory_order_relaxed);
        } else if (++wake_hint_ % 1024 == 0) {
            cv_.notify_one();
        }
    }

    // 便捷宏风格
    template <typename... Args>
    void info(const char* fmt, Args... args) { logf(Level::Info, fmt, args...); }
    template <typename... Args>
    void warn(const char* fmt, Args... args) { logf(Level::Warn, fmt, args...); }
    template <typename... Args>
    void error(const char* fmt, Args... args) { logf(Level::Error, fmt, args...); }

    // 统计
    uint64_t dropped_total() const {
        uint64_t s = 0;
        std::lock_guard<std::mutex> lk(mu_);
        for (auto* p : producers_) s += p->dropped.load(std::memory_order_relaxed);
        return s;
    }

private:
    AsyncLogger() = default;
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    static uint64_t now_ns_coarse() {
#ifdef CLOCK_REALTIME_COARSE
        timespec ts; clock_gettime(CLOCK_REALTIME_COARSE, &ts);
#else
        timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
#endif
        return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
    }
    static uint32_t thread_id_hash() {
        auto id = std::this_thread::get_id();
        return static_cast<uint32_t>(std::hash<std::thread::id>{}(id));
    }

    Producer* get_or_create_producer() {
        thread_local Producer* tls = nullptr;
        if (tls) return tls;
        auto p = std::make_unique<Producer>();
        tls = p.get();
        {
            std::lock_guard<std::mutex> lk(mu_);
            producers_.push_back(tls);
            owned_.push_back(std::move(p));
        }
        return tls;
    }

    // 后台线程
    void run() {
        uint64_t last_flush_ns = now_ns_coarse();
        size_t   bytes_since_flush = 0;

        while (true) {
            if (!running_) {
                drain_all(bytes_since_flush);
                // final flush
                file_sink_->flush(true);
                console_sink_->flush(true);
                break;
            }

            bool did = drain_all(bytes_since_flush);

            uint64_t now = now_ns_coarse();
            bool time_to_flush =
                (now - last_flush_ns) >= (uint64_t(cfg_.flush_interval.count()) * 1000000ull);

            if (bytes_since_flush >= cfg_.flush_bytes || time_to_flush) {
                file_sink_->flush(false);
                console_sink_->flush(false);
                bytes_since_flush = 0;
                last_flush_ns = now;
            }

            if (!did) {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait_for(lk, std::chrono::microseconds(200));
            }
        }
    }

    bool drain_all(size_t& bytes_since_flush) {
        bool did = false;
        std::vector<Producer*> local;
        {
            std::lock_guard<std::mutex> lk(mu_);
            local = producers_;
        }
        for (auto* p : local) {
            Entry e;
            int round = 0;
            while (p->ring->try_pop(e)) {
                did = true;
                // 一次格式化，多路写
                std::string line = format_line(e);
                LineView lv{ line.data(), line.size(), static_cast<Level>(e.level) };
                file_sink_->write(lv);
                console_sink_->write(lv);
                bytes_since_flush += lv.size;
                if (++round >= 1024) break;
            }
        }
        return did;
    }

    // 格式化一条日志为行文本（只做一次）
    std::string format_line(const Entry& e) {
        char tbuf[32]; format_time(e.ts_ns, tbuf, sizeof(tbuf));
        char tidbuf[16]; int n = snprintf(tidbuf, sizeof(tidbuf), "%u", e.tid);
        std::string out;
        out.reserve(e.len + 128);
        out.append("["); out.append(tbuf); out.append("][");
        out.append(level_name(e.level)); out.append("][tid:");
        out.append(tidbuf, (n>0)?(size_t)n:0);
        out.append("]["); out.append(e.file); out.append(":");
        char lbuf[16]; snprintf(lbuf, sizeof(lbuf), "%d", e.line);
        out.append(lbuf);
        out.append(" "); out.append(e.func);
        out.append("] ");
        out.append(e.msg, e.len);
        out.push_back('\n');
        return out;
    }

    static void format_time(uint64_t ns, char* buf, size_t N) {
        // 确保缓冲区足够大
        if (N < 26) { // 26 是最大长度
            // 处理错误，例如返回
            if (buf) {
                buf[0] = '\0'; // 清空缓冲区
            }
            return; // 或者抛出异常，或者返回错误代码
        }

        time_t sec = ns / 1000000000ull;
        long us = (ns % 1000000000ull) / 1000;
        tm tmv;
        localtime_r(&sec, &tmv);

        // 使用 snprintf 进行格式化
        int ret = snprintf(buf, N, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
                           tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                           tmv.tm_hour, tmv.tm_min, tmv.tm_sec, us);

        // 检查返回值
        if (ret < 0) {
            // 处理 snprintf 失败的情况
            perror("snprintf failed");
        } else if (ret >= static_cast<int>(N)) {
            // 处理截断的情况
            fprintf(stderr, "Warning: output truncated, wrote %d characters\n", ret);
        }
    }

    void append_line(std::string& out, const Entry& e) {
        char tbuf[32]; format_time(e.ts_ns, tbuf, sizeof(tbuf));
        // 典型行格式：[time][lvl][tid] message\n
        out.append("[", 1);
        out.append(tbuf);
        out.append("][", 2);
        out.append(level_name(e.level));
        out.append("][tid:", 6);
        char tidbuf[16]; int n = snprintf(tidbuf, sizeof(tidbuf), "%u", e.tid);
        out.append(tidbuf, (n>0)?(size_t)n:0);
        out.append("] ", 2);
        out.append(e.msg, e.len);
        out.push_back('\n');
        written_in_file_ += (out.size() - last_size_snapshot_);
        last_size_snapshot_ = out.size();
    }

    void flush(std::string& out, size_t& bytes_since_flush, bool final) {
        if (out.empty()) return;
        ssize_t n = ::write(fd_, out.data(), out.size());
        (void)n; // 简化处理，生产中请检查 n
        if (cfg_.fsync_on_flush || final) ::fsync(fd_);
        out.clear();
        last_size_snapshot_ = 0;
        bytes_since_flush = 0;
    }

    void open_file_locked() {
        if (fd_ >= 0) ::close(fd_);
        fd_ = ::open(cfg_.path.c_str(), O_CREAT|O_APPEND|O_WRONLY|O_CLOEXEC, 0644);
        written_in_file_ = 0;
    }

    void rotate() {
        std::lock_guard<std::mutex> lk(mu_);
        // 生成新名：path.YYYYmmdd-HHMMSS.pid
        char tbuf[32];
        uint64_t ns = now_ns_coarse();
        time_t sec = ns / 1000000000ull;
        tm tmv; localtime_r(&sec, &tmv);
        snprintf(tbuf, sizeof(tbuf), "%04d%02d%02d-%02d%02d%02d",
                 tmv.tm_year+1900, tmv.tm_mon+1, tmv.tm_mday,
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        char newname[1024];
        pid_t pid = getpid();
        snprintf(newname, sizeof(newname), "%s.%s.%d",
                 cfg_.path.c_str(), tbuf, (int)pid);
        ::close(fd_);
        ::rename(cfg_.path.c_str(), newname);
        open_file_locked();
    }

private:
    // 状态
    Config cfg_;

    // sink
    std::unique_ptr<FileSink>    file_sink_;
    std::unique_ptr<ConsoleSink> console_sink_;

    std::atomic<bool> running_{false};
    std::thread worker_;
    int fd_ = -1;

    // 生产者集合
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::vector<Producer*> producers_;
    std::vector<std::unique_ptr<Producer>> owned_;

    // 写统计
    size_t written_in_file_ = 0;
    size_t last_size_snapshot_ = 0;
    std::atomic<uint32_t> wake_hint_{0};
};

// #define LOG_TRACE(fmt, ...) AsyncLogger<>::instance().logf(Level::Trace, fmt, ##__VA_ARGS__)
// #define LOG_DEBUG(fmt, ...) AsyncLogger<>::instance().logf(Level::Debug, fmt, ##__VA_ARGS__)
// #define LOG_INFO(fmt, ...)  AsyncLogger<>::instance().logf(Level::Info,  fmt, ##__VA_ARGS__)
// #define LOG_WARN(fmt, ...)  AsyncLogger<>::instance().logf(Level::Warn,  fmt, ##__VA_ARGS__)
// #define LOG_ERROR(fmt, ...) AsyncLogger<>::instance().logf(Level::Error, fmt, ##__VA_ARGS__)
// #define LOG_FATAL(fmt, ...) AsyncLogger<>::instance().logf(Level::Fatal, fmt, ##__VA_ARGS__)

#define LOG_TRACE(fmt, ...) AsyncLogger<>::instance()._logf_impl(Level::Trace, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  AsyncLogger<>::instance()._logf_impl(Level::Info, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  AsyncLogger<>::instance()._logf_impl(Level::Warn, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) AsyncLogger<>::instance()._logf_impl(Level::Error, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) AsyncLogger<>::instance()._logf_impl(Level::Debug, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) AsyncLogger<>::instance()._logf_impl(Level::Fatal, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif //LOG_H
