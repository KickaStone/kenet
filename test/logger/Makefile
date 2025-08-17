CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 -pthread
LDFLAGS = -pthread

# 目标文件
TARGETS = logger_test bench

.PHONY: all clean

all: $(TARGETS)

# 编译基础压测客户端
press_client: logger_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

bench: logger_bench.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f *.log