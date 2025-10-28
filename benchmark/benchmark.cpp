// benchmark_client.cpp
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

constexpr int THREADS = 8;
constexpr int CONN_PER_THREAD = 100;
constexpr int MSG_SIZE = 128;
constexpr int REQ_PER_CONN = 10000;

void worker(int tid) {
    for (int i = 0; i < CONN_PER_THREAD; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9000);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(fd, (sockaddr*)&addr, sizeof(addr));

        std::string msg(MSG_SIZE, 'x');
        char buf[MSG_SIZE];
        for (int j = 0; j < REQ_PER_CONN; ++j) {
            send(fd, msg.data(), msg.size(), 0);
            recv(fd, buf, msg.size(), MSG_WAITALL);
        }
        close(fd);
    }
    std::cout << "Thread " << tid << " done\n";
}

int main() {
    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> ths;
    for (int i = 0; i < THREADS; ++i)
        ths.emplace_back(worker, i);
    for (auto &t : ths) t.join();
    auto end = std::chrono::steady_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();
    double total = 1.0 * THREADS * CONN_PER_THREAD * REQ_PER_CONN;
    std::cout << "Total: " << total / sec << " req/s\n";
}