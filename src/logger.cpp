#include "cpphttp/logger.hpp"
#include <sstream>

namespace cpphttp {

// ANSI colour codes
const char* Logger::level_color(LogLevel l) {
    switch (l) {
        case LogLevel::DEBUG: return "\033[36m";   // Cyan
        case LogLevel::INFO:  return "\033[32m";   // Green
        case LogLevel::WARN:  return "\033[33m";   // Yellow
        case LogLevel::ERROR: return "\033[31m";   // Red
        default:              return "\033[0m";
    }
}

const char* Logger::level_str(LogLevel l) {
    switch (l) {
        case LogLevel::DEBUG: return "DBG";
        case LogLevel::INFO:  return "INF";
        case LogLevel::WARN:  return "WRN";
        case LogLevel::ERROR: return "ERR";
        default:              return "???";
    }
}

std::string Logger::now_str() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_info{};
#ifdef _WIN32
    localtime_s(&tm_info, &t);
#else
    localtime_r(&t, &tm_info);
#endif
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
    return buf;
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < min_level_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    std::cerr
        << level_color(level)
        << "[" << now_str() << "] "
        << level_str(level) << " "
        << "\033[0m"
        << msg << "\n";
}

void Logger::access(const std::string& method, const std::string& path,
                    int status, long duration_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Status colour
    const char* sc = (status < 300) ? "\033[32m"
                   : (status < 400) ? "\033[36m"
                   : (status < 500) ? "\033[33m"
                                    : "\033[31m";

    std::cerr
        << "\033[90m" << "[" << now_str() << "] \033[0m"
        << "\033[1m"  << method << "\033[0m"
        << " "        << path
        << " "        << sc << status << "\033[0m"
        << " \033[90m" << duration_ms << "ms\033[0m"
        << "\n";
}

} // namespace cpphttp
