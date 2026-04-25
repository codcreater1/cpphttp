#pragma once

#include <string>
#include <mutex>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace cpphttp {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

/**
 * @brief Thread-safe, colour-aware logger.
 */
class Logger {
public:
    void set_level(LogLevel level) { min_level_ = level; }

    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info (const std::string& msg) { log(LogLevel::INFO,  msg); }
    void warn (const std::string& msg) { log(LogLevel::WARN,  msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }

    // Log an incoming request (method, path, status, duration_ms)
    void access(const std::string& method, const std::string& path,
                int status, long duration_ms);

private:
    void log(LogLevel level, const std::string& msg);

    static const char* level_str(LogLevel l);
    static const char* level_color(LogLevel l);
    static std::string now_str();

    LogLevel   min_level_ = LogLevel::INFO;
    std::mutex mutex_;
};

} // namespace cpphttp
