#ifndef SINK_H
#define SINK_H
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <ctime>

enum class Level : uint8_t { Trace, Debug, Info, Warn, Error, Fatal, Off /* 不输出 */ };

inline const char* level_name(Level lv) {
    switch (lv) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        default:           return "OFF";
    }
}

struct LineView {
    const char* data;
    size_t      size;
    Level       level;
};

struct ISink {
    virtual ~ISink() = default;
    Level min_level{Level::Info};
    virtual void write(LineView lv) = 0;      // 仅写，暂不刷盘
    virtual void flush(bool final) = 0;       // 刷新；final=true 用于 stop()
};


// ---- FileSink: 追加写 & 可控 fsync ----
class FileSink : public ISink {
public:
    explicit FileSink(std::string path, bool fsync_on_flush, size_t rotate_bytes)
        : path_(std::move(path)), fsync_on_flush_(fsync_on_flush), rotate_bytes_(rotate_bytes) {
        open_file();
    }

    ~FileSink() override { if (fd_ >= 0) ::close(fd_); }

    void write(LineView lv) override {
        if (lv.level < min_level) return;
        buf_.append(lv.data, lv.size);
        written_ += lv.size;
        if (written_ >= rotate_bytes_) rotate();
    }

    void flush(bool final) override {
        if (buf_.empty()) {
            if (final && fsync_on_flush_ && fd_ >= 0) ::fsync(fd_);
            return;
        }
        if (const ssize_t bytes_written = ::write(fd_, buf_.data(), buf_.size()); bytes_written == -1) {
            perror("write failed.");
        }else if (bytes_written < static_cast<ssize_t>(buf_.size())) {
            std::cerr << "Warning: Only " << bytes_written << " out of " << " bytes written." << std::endl;
            // do nothing, only record
        }
        if (fsync_on_flush_ || final) ::fsync(fd_);
        buf_.clear();
    }

private:
    void open_file() {
        if (fd_ >= 0) ::close(fd_);
        fd_ = ::open(path_.c_str(), O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, 0644);
        written_ = 0;
    }

    void rotate() {
        // 简单基于时间戳的轮转
        char tbuf[32];
        time_t sec = time(nullptr);
        tm tmv;
        localtime_r(&sec, &tmv);
        snprintf(tbuf, sizeof(tbuf), "%04d%02d%02d-%02d%02d%02d",
                 tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        char newname[1024];
        snprintf(newname, sizeof(newname), "%s.%s.%d", path_.c_str(), tbuf, (int) getpid());
        ::close(fd_);
        ::rename(path_.c_str(), newname);
        open_file();
    }

    std::string path_;
    bool fsync_on_flush_{false};
    size_t rotate_bytes_{128ull << 20};

    int fd_{-1};
    std::string buf_;
    size_t written_{0};
};

// ---- ConsoleSink: 输出到 stderr，支持 ANSI 颜色 ----
class ConsoleSink : public ISink {
public:
    explicit ConsoleSink(bool enable_color = true): color_(enable_color) {
    }

    void write(LineView lv) override {
        if (lv.level < min_level) return;
        if (!color_) {
            buf_.append(lv.data, lv.size);
            return;
        }
        // 简单着色：Warn/Yellow, Error/Red, Fatal/Bold Red
        const char *pre = "", *suf = "\033[0m";
        switch (lv.level) {
            case Level::Warn: pre = "\033[33m";
                break;
            case Level::Error: pre = "\033[31m";
                break;
            case Level::Fatal: pre = "\033[1;31m";
                break;
            default: pre = "";
                suf = "";
                break;
        }
        if (*pre) buf_.append(pre);
        buf_.append(lv.data, lv.size);
        if (*pre) buf_.append(suf);
    }

    void flush(bool) override {
        if (buf_.empty()) return;
        if (const ssize_t bytes_written =  ::write(STDOUT_FILENO, buf_.data(), buf_.size()); bytes_written == -1) {
            perror("write failed.");
        }else if (bytes_written < static_cast<ssize_t>(buf_.size())) {
            std::cerr << "Warning: Only " << bytes_written << " out of " << " bytes written." << std::endl;
        }
        buf_.clear();
    }

private:
    bool color_{true};
    std::string buf_;
};


#endif //SINK_H
