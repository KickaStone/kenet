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
#include <map>
#include <random>
#include <errno.h>
#include <csignal>

class ContinuousPressClient {
public:
    struct Config {
        std::string server_ip = "127.0.0.1";
        int server_port = 9000;
        int max_connections = 1000;        // 最大连接数
        int connections_per_second = 10;   // 每秒新建连接数
        int connection_keep_time_ms = 5000; // 连接保持时长(毫秒)
        int send_interval_ms = 1000;       // 发送数据间隔(毫秒)
        bool verbose = false;              // 是否输出详细信息
    };

    ContinuousPressClient(const Config& config) 
        : config_(config), running_(true), total_connections_(0), 
          active_connections_(0), successful_requests_(0), failed_requests_(0),
          data_mismatch_count_(0), connection_errors_(0) {
        
        // 初始化随机数生成器
        rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    void run() {
        std::cout << "Starting continuous pressure test..." << std::endl;
        std::cout << "Server: " << config_.server_ip << ":" << config_.server_port << std::endl;
        std::cout << "Max connections: " << config_.max_connections << std::endl;
        std::cout << "Connections per second: " << config_.connections_per_second << std::endl;
        std::cout << "Connection keep time: " << config_.connection_keep_time_ms << " ms" << std::endl;
        std::cout << "Send interval: " << config_.send_interval_ms << " ms" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        // 启动连接创建线程
        std::thread connection_creator(&ContinuousPressClient::connection_creator_thread, this);
        
        // 启动统计输出线程
        std::thread stats_thread(&ContinuousPressClient::stats_thread, this);
        
        // 等待线程结束
        connection_creator.join();
        stats_thread.join();
    }
    
    void stop() {
        running_.store(false);
    }

private:
    struct Connection {
        int fd;
        std::chrono::steady_clock::time_point created_time;
        std::chrono::steady_clock::time_point last_send_time;
        int request_count;
        std::string last_sent_data;
    };
    
    void connection_creator_thread() {
        auto last_connection_time = std::chrono::steady_clock::now();
        const auto connection_interval = std::chrono::milliseconds(1000 / config_.connections_per_second);
        
        while (running_.load()) {
            auto now = std::chrono::steady_clock::now();
            
            // 检查是否需要创建新连接
            if (active_connections_.load() < config_.max_connections && 
                now - last_connection_time >= connection_interval) {
                
                create_connection();
                last_connection_time = now;
            }
            
            // 管理现有连接
            manage_connections();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 清理所有连接
        cleanup_all_connections();
    }
    
    void create_connection() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            connection_errors_++;
            return;
        }
        
        // 设置socket选项
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // 设置超时
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(config_.server_port);
        server_addr.sin_addr.s_addr = inet_addr(config_.server_ip.c_str());
        
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            connection_errors_++;
            return;
        }
        
        // 创建连接对象
        Connection conn;
        conn.fd = sockfd;
        conn.created_time = std::chrono::steady_clock::now();
        conn.last_send_time = conn.created_time;
        conn.request_count = 0;
        
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_[sockfd] = conn;
        }
        
        active_connections_++;
        total_connections_++;
    }
    
    void manage_connections() {
        auto now = std::chrono::steady_clock::now();
        std::vector<int> to_remove;
        
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            
            for (auto& pair : connections_) {
                int fd = pair.first;
                Connection& conn = pair.second;
                
                // 检查连接是否超时
                auto connection_age = now - conn.created_time;
                if (connection_age >= std::chrono::milliseconds(config_.connection_keep_time_ms)) {
                    to_remove.push_back(fd);
                    continue;
                }
                
                // 检查是否需要发送数据
                auto time_since_last_send = now - conn.last_send_time;
                if (time_since_last_send >= std::chrono::milliseconds(config_.send_interval_ms)) {
                    if (send_and_verify_data(conn)) {
                        successful_requests_++;
                    } else {
                        failed_requests_++;
                        // 如果发送失败，标记连接为需要移除
                        to_remove.push_back(fd);
                    }
                    conn.last_send_time = now;
                }
            }
        }
        
        // 移除超时或失败的连接
        for (int fd : to_remove) {
            remove_connection(fd);
        }
    }
    
    bool send_and_verify_data(Connection& conn) {
        // 生成测试数据
        std::string test_data = generate_test_data();
        conn.last_sent_data = test_data;
        
        // 发送数据
        ssize_t sent = send(conn.fd, test_data.c_str(), test_data.length(), 0);
        if (sent < 0) {
            return false;
        }
        
        // 接收响应
        char buffer[1024];
        ssize_t received = recv(conn.fd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            return false;
        }
        
        buffer[received] = '\0';
        
        // 验证响应是否正确
        if (std::string(buffer) == test_data) {
            conn.request_count++;
            return true;
        } else {
            data_mismatch_count_++;
            return false;
        }
    }
    
    std::string generate_test_data() {
        // 生成随机测试数据
        std::uniform_int_distribution<int> length_dist(10, 100);
        std::uniform_int_distribution<int> char_dist(32, 126); // 可打印ASCII字符
        
        int length = length_dist(rng_);
        std::string data;
        data.reserve(length);
        
        for (int i = 0; i < length; ++i) {
            data += static_cast<char>(char_dist(rng_));
        }
        
        return data;
    }
    
    void remove_connection(int fd) {
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.erase(fd);
        }
        
        close(fd);
        active_connections_--;
    }
    
    void cleanup_all_connections() {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& pair : connections_) {
            close(pair.first);
        }
        connections_.clear();
        active_connections_ = 0;
    }
    
    void stats_thread() {
        auto start_time = std::chrono::steady_clock::now();
        auto last_stats_time = start_time;
        int last_total_connections = 0;
        int last_successful_requests = 0;
        int last_failed_requests = 0;
        
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            auto now = std::chrono::steady_clock::now();
            auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            auto stats_duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time);
            
            int current_total_connections = total_connections_.load();
            int current_successful_requests = successful_requests_.load();
            int current_failed_requests = failed_requests_.load();
            int current_active_connections = active_connections_.load();
            int current_data_mismatch = data_mismatch_count_.load();
            int current_connection_errors = connection_errors_.load();
            
            // 计算速率
            int connections_per_sec = (current_total_connections - last_total_connections) / stats_duration.count();
            int requests_per_sec = (current_successful_requests + current_failed_requests - 
                                  last_successful_requests - last_failed_requests) / stats_duration.count();
            
            std::cout << "[" << total_duration.count() << "s] "
                      << "Active: " << current_active_connections 
                      << " | Total: " << current_total_connections
                      << " | Conn/s: " << connections_per_sec
                      << " | Req/s: " << requests_per_sec
                      << " | Success: " << current_successful_requests
                      << " | Failed: " << current_failed_requests
                      << " | DataMismatch: " << current_data_mismatch
                      << " | ConnErrors: " << current_connection_errors << std::endl;
            
            last_stats_time = now;
            last_total_connections = current_total_connections;
            last_successful_requests = current_successful_requests;
            last_failed_requests = current_failed_requests;
        }
    }
    
    Config config_;
    std::atomic<bool> running_;
    std::atomic<int> total_connections_;
    std::atomic<int> active_connections_;
    std::atomic<int> successful_requests_;
    std::atomic<int> failed_requests_;
    std::atomic<int> data_mismatch_count_;
    std::atomic<int> connection_errors_;
    
    std::mutex connections_mutex_;
    std::map<int, Connection> connections_;
    std::mt19937 rng_;
};

// 全局变量用于信号处理
ContinuousPressClient* g_client = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_client) {
        g_client->stop();
    }
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                    Show this help message" << std::endl;
    std::cout << "  -s, --server <ip>             Server IP address (default: 127.0.0.1)" << std::endl;
    std::cout << "  -p, --port <port>             Server port (default: 9000)" << std::endl;
    std::cout << "  -m, --max-connections <num>   Maximum concurrent connections (default: 1000)" << std::endl;
    std::cout << "  -c, --connections-per-sec <num> Connections per second (default: 10)" << std::endl;
    std::cout << "  -k, --keep-time <ms>          Connection keep time in milliseconds (default: 5000)" << std::endl;
    std::cout << "  -i, --send-interval <ms>      Send interval in milliseconds (default: 1000)" << std::endl;
    std::cout << "  -v, --verbose                 Verbose output" << std::endl;
    std::cout << std::endl;
    std::cout << "Example: " << program_name << " -s 127.0.0.1 -p 9000 -m 500 -c 20 -k 3000" << std::endl;
}

int main(int argc, char* argv[]) {
    ContinuousPressClient::Config config;
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-s" || arg == "--server") {
            if (i + 1 < argc) {
                config.server_ip = argv[++i];
            } else {
                std::cerr << "Error: --server requires an IP address" << std::endl;
                return 1;
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.server_port = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --port requires a port number" << std::endl;
                return 1;
            }
        } else if (arg == "-m" || arg == "--max-connections") {
            if (i + 1 < argc) {
                config.max_connections = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --max-connections requires a number" << std::endl;
                return 1;
            }
        } else if (arg == "-c" || arg == "--connections-per-sec") {
            if (i + 1 < argc) {
                config.connections_per_second = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --connections-per-sec requires a number" << std::endl;
                return 1;
            }
        } else if (arg == "-k" || arg == "--keep-time") {
            if (i + 1 < argc) {
                config.connection_keep_time_ms = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --keep-time requires a number" << std::endl;
                return 1;
            }
        } else if (arg == "-i" || arg == "--send-interval") {
            if (i + 1 < argc) {
                config.send_interval_ms = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --send-interval requires a number" << std::endl;
                return 1;
            }
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 验证配置
    if (config.max_connections <= 0 || config.connections_per_second <= 0 || 
        config.connection_keep_time_ms <= 0 || config.send_interval_ms <= 0) {
        std::cerr << "Error: All numeric parameters must be positive" << std::endl;
        return 1;
    }
    
    // 注册信号处理函数
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // 终止信号
    
    // 创建并运行客户端
    ContinuousPressClient client(config);
    g_client = &client;
    
    try {
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    g_client = nullptr;
    return 0;
}
