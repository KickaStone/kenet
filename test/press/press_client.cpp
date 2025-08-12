#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <errno.h> // Required for errno

class PressClient {
public:
    PressClient(const std::string& ip, int port, int num_threads, int requests_per_thread)
        : server_ip_(ip), server_port_(port), num_threads_(num_threads), 
          requests_per_thread_(requests_per_thread), total_requests_(0), 
          success_count_(0), fail_count_(0) {}
    
    void run() {
        std::cout << "Starting pressure test..." << std::endl;
        std::cout << "Server: " << server_ip_ << ":" << server_port_ << std::endl;
        std::cout << "Threads: " << num_threads_ << std::endl;
        std::cout << "Requests per thread: " << requests_per_thread_ << std::endl;
        std::cout << "Total requests: " << num_threads_ * requests_per_thread_ << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads_; ++i) {
            threads.emplace_back(&PressClient::worker_thread, this, i);
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        print_results(duration.count());
    }
    
private:
    void worker_thread(int thread_id) {
        for (int i = 0; i < requests_per_thread_; ++i) {
            if (send_request(thread_id, i)) {
                success_count_++;
            } else {
                fail_count_++;
            }
            total_requests_++;
        }
    }
    
    bool send_request(int thread_id, int request_id) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Thread " << thread_id << " Request " << request_id << ": socket() failed: " << strerror(errno) << std::endl;
            return false;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port_);
        server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
        
        // 设置连接超时
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Thread " << thread_id << " Request " << request_id << ": connect() failed: " << strerror(errno) << std::endl;
            close(sockfd);
            return false;
        }
        
        // 构造测试消息
        std::string message = "Thread-" + std::to_string(thread_id) + 
                             "-Request-" + std::to_string(request_id) + 
                             "-Hello from pressure test!";
        
        ssize_t sent = send(sockfd, message.c_str(), message.length(), 0);
        if (sent < 0) {
            std::cerr << "Thread " << thread_id << " Request " << request_id << ": send() failed: " << strerror(errno) << std::endl;
            close(sockfd);
            return false;
        }
        
        // 接收响应 - 这是关键
        char buffer[1024];
        ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        close(sockfd);
        
        if (received > 0) {
            buffer[received] = '\0';
            // 验证响应是否正确
            if (std::string(buffer) == message) {
                return true;  // 只有收到正确响应才算成功
            } else {
                std::cerr << "Thread " << thread_id << " Request " << request_id << ": response mismatch" << std::endl;
                return false;
            }
        } else {
            // 任何接收失败都算失败
            return false;
        }
    }
    
    void print_results(long long duration_ms) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Pressure test completed!" << std::endl;
        std::cout << "Duration: " << duration_ms << " ms" << std::endl;
        std::cout << "Total requests: " << total_requests_.load() << std::endl;
        std::cout << "Successful: " << success_count_.load() << std::endl;
        std::cout << "Failed: " << fail_count_.load() << std::endl;
        std::cout << "Success rate: " << (success_count_.load() * 100.0 / total_requests_.load()) << "%" << std::endl;
        std::cout << "QPS: " << (total_requests_.load() * 1000.0 / duration_ms) << std::endl;
        std::cout << "Average response time: " << (duration_ms * 1.0 / total_requests_.load()) << " ms" << std::endl;
    }
    
    std::string server_ip_;
    int server_port_;
    int num_threads_;
    int requests_per_thread_;
    std::atomic<int> total_requests_;
    std::atomic<int> success_count_;
    std::atomic<int> fail_count_;
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <server_ip> <server_port> [threads] [requests_per_thread]" << std::endl;
    std::cout << "  server_ip: Server IP address (default: 127.0.0.1)" << std::endl;
    std::cout << "  server_port: Server port (default: 9000)" << std::endl;
    std::cout << "  threads: Number of threads (default: 10)" << std::endl;
    std::cout << "  requests_per_thread: Requests per thread (default: 100)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example: " << program_name << " 127.0.0.1 9000 20 50" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string server_ip = "127.0.0.1";
    int server_port = 9000;
    int num_threads = 10;
    int requests_per_thread = 100;
    
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        server_ip = argv[1];
    }
    
    if (argc > 2) {
        server_port = std::stoi(argv[2]);
    }
    
    if (argc > 3) {
        num_threads = std::stoi(argv[3]);
    }
    
    if (argc > 4) {
        requests_per_thread = std::stoi(argv[4]);
    }
    
    if (num_threads <= 0 || requests_per_thread <= 0) {
        std::cerr << "Error: threads and requests_per_thread must be positive integers" << std::endl;
        return 1;
    }
    
    PressClient client(server_ip, server_port, num_threads, requests_per_thread);
    client.run();
    
    return 0;
}