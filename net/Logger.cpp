#include "Logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

void Logger::Log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(INFO, format, args);
    va_end(args);
}

void Logger::Log(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(level, format, args);
    va_end(args);
}

void Logger::Debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(DEBUG, format, args);
    va_end(args);
}

void Logger::Info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(INFO, format, args);
    va_end(args);
}

void Logger::Warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(WARN, format, args);
    va_end(args);
}

void Logger::Error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(ERROR, format, args);
    va_end(args);
}

void Logger::Fatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogInternal(FATAL, format, args);
    va_end(args);
}

void Logger::LogInternal(LogLevel level, const char* format, va_list args) {
    // 获取当前时间
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // 输出时间戳和日志级别
    printf("[%s] [%s] ", timeStr, GetLevelString(level));
    
    // 使用vprintf安全地输出格式化字符串
    vprintf(format, args);
    printf("\n");
    fflush(stdout); // 确保立即输出
}

const char* Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default:    return "UNKNOWN";
    }
}