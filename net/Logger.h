#ifndef _NET_LOGGER_H_
#define _NET_LOGGER_H_

#include <string>
#include <iostream>
#include <cstdarg>

enum LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static void Log(const char* format, ...);
    static void Log(LogLevel level, const char* format, ...);
    static void Debug(const char* format, ...);
    static void Info(const char* format, ...);
    static void Warn(const char* format, ...);
    static void Error(const char* format, ...);
    static void Fatal(const char* format, ...);
    
private:
    static void LogInternal(LogLevel level, const char* format, va_list args);
    static const char* GetLevelString(LogLevel level);
};

#endif