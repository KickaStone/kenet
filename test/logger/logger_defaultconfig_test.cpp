#include "../../include/logger/log.h"
#include <iostream>

int main() {
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");

    LOG_INFO("Server started on %s:%d", "localhost", 8080);
    LOG_WARN("Memory usage: %.1f%%", 85.5);
    return 0;
}
