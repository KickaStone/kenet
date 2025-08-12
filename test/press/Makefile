CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 -pthread
LDFLAGS = -pthread

# 目标文件
TARGETS = press_client

.PHONY: all clean

all: $(TARGETS)

# 编译基础压测客户端
press_client: press_client.cpp
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

clean:
	rm -f $(TARGETS)

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all            - Build all targets"
	@echo "  press_client   - Build basic pressure test client"
	@echo ""
	@echo "  run            - Run basic pressure test with default parameters"
	@echo "  test           - Run light basic pressure test (10 threads, 100 requests each)"
	@echo "  stress         - Run heavy basic pressure test (50 threads, 200 requests each)"
	@echo "  clean          - Remove built files"
	@echo ""
	@echo "Basic usage: ./press_client <server_ip> <server_port> [threads] [requests_per_thread]"
	@echo ""
	@echo "Examples:"
	@echo "  ./press_client 127.0.0.1 9000 20 50"
