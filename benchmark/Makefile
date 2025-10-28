CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 -pthread
LDFLAGS = -pthread

TARGETS = benchmark

.PHONY: all clean

all: $(TARGETS)

benchmark: benchmark.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)