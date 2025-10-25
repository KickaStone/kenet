CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 -pthread -static -static-libgcc -static-libstdc++
LDFLAGS = -pthread

# 目标文件
TARGETS = press_client continuous_press_client

.PHONY: all clean

all: $(TARGETS)

# 编译基础压测客户端
press_client: press_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# 编译持续压测客户端
continuous_press_client: continuous_press_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# 编译并运行基础压测
run: press_client
	./press_client

# 运行示例压测
test: press_client
	./press_client 127.0.0.1 9000 10 100

# 高强度压测
stress: press_client
	./press_client 127.0.0.1 9000 50 200

# 持续压测
continuous: continuous_press_client
	./continuous_press_client

# 持续压测示例
continuous-test: continuous_press_client
	./continuous_press_client -s 127.0.0.1 -p 9000 -m 100 -c 5 -k 3000

# 高强度持续压测
continuous-stress: continuous_press_client
	./continuous_press_client -s 127.0.0.1 -p 9000 -m 500 -c 20 -k 5000

clean:
	rm -f $(TARGETS)

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all                    - Build all targets"
	@echo "  press_client           - Build basic pressure test client"
	@echo "  continuous_press_client - Build continuous pressure test client"
	@echo ""
	@echo "Basic pressure test:"
	@echo "  run                    - Run basic pressure test with default parameters"
	@echo "  test                   - Run light basic pressure test (10 threads, 100 requests each)"
	@echo "  stress                 - Run heavy basic pressure test (50 threads, 200 requests each)"
	@echo ""
	@echo "Continuous pressure test:"
	@echo "  continuous             - Run continuous pressure test with default parameters"
	@echo "  continuous-test        - Run light continuous pressure test (100 max conn, 5 conn/s)"
	@echo "  continuous-stress      - Run heavy continuous pressure test (500 max conn, 20 conn/s)"
	@echo ""
	@echo "  clean                  - Remove built files"
	@echo ""
	@echo "Basic usage: ./press_client <server_ip> <server_port> [threads] [requests_per_thread]"
	@echo "Continuous usage: ./continuous_press_client [options]"
	@echo ""
	@echo "Examples:"
	@echo "  ./press_client 127.0.0.1 9000 20 50"
	@echo "  ./continuous_press_client -s 127.0.0.1 -p 9000 -m 200 -c 10 -k 5000"
