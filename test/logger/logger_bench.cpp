#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "../../include/logger/log.h"

int main() {
    using Logger = AsyncLogger<512, 4096>;
    Config cfg;
    cfg.path = "server.log";
    cfg.rotate_bytes = 200ull << 20;     // 200MB
    cfg.flush_bytes  = 128ull << 10;     // 128KB
    cfg.flush_interval = std::chrono::milliseconds(200);
    cfg.fsync_on_flush = false;          // 需要更强可靠可设 true

    auto& L = Logger::instance();
    L.start(cfg);

    const int rounds = 10; // 轮数
    const int logs_per_round = 1000000; // 每轮日志条数
    const int num_threads = 4; // 线程数

    for (int round = 1; round <= rounds; ++round) {
        std::vector<std::thread> ths;
        auto start_time = std::chrono::high_resolution_clock::now();

        for (int t = 0; t < num_threads; ++t) {
            ths.emplace_back([t, &L, logs_per_round, num_threads] {
                for (int i = 0; i < logs_per_round / num_threads; ++i) {
                    L.info("worker=%d value=%d hello async logger!", t, i);
                }
            });
        }

        for (auto& th : ths) th.join();

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;

        std::cout << "Round " << round << ": " << duration.count() << " ms to log "
                  << logs_per_round << " messages." << std::endl;
    }

    L.stop();
    return 0;
}
