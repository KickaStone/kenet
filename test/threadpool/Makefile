CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 -pthread
LDFLAGS = -pthread

# 目标文件
TARGETS = threadpool_test

.PHONY: all clean

all: $(TARGETS)

# 编译基础压测客户端
threadpool_test: threadpool_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
