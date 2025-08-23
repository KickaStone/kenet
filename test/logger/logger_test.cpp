// main.cpp
#include "../../include/logger/log.h"
#include <thread>

int main() {
    using Logger = AsyncLogger<512, 4096>;
    Config cfg;
    cfg.path = "server.log";
    cfg.file_level = Level::Debug;   // 文件从 Debug 开始记
    cfg.console_level = Level::Info; // 控制台只打 Warn+
    cfg.flush_bytes = 128<<10;
    cfg.flush_interval = std::chrono::milliseconds(100);
    cfg.console_enable_color = true;

    auto& L = Logger::instance();
    L.start(cfg);

    L.info ("listen on %s:%d", "0.0.0.0", 8080);
    L.warn ("connection backlog high: %d", 1024);
    L.error("disk almost full: %.1f%%", 97.3);

    L.stop();
}
