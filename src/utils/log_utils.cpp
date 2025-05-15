/**
 * @file log_utils.cpp
 * @brief 日志工具函数实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/string_utils.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <cstdarg>

namespace dlogcover {
namespace utils {

// 静态成员变量
namespace {
    LogLevel currentLogLevel = LogLevel::INFO;
    std::string logFilePath;
    bool enableConsoleOutput = true;
    std::mutex logMutex;
    std::ofstream logFileStream;
    bool isInitialized = false;
}

bool Logger::init(const std::string& logFileName, bool consoleOutput, LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // 关闭已打开的日志文件
    if (logFileStream.is_open()) {
        logFileStream.close();
    }
    
    // 设置日志级别
    currentLogLevel = level;
    
    // 设置控制台输出
    enableConsoleOutput = consoleOutput;
    
    // 设置日志文件路径
    logFilePath = logFileName;
    
    // 打开日志文件
    logFileStream.open(logFilePath, std::ios::out | std::ios::app);
    if (!logFileStream.is_open()) {
        if (enableConsoleOutput) {
            std::cerr << "无法打开日志文件: " << logFilePath << std::endl;
        }
        return false;
    }
    
    isInitialized = true;
    
    // 记录初始化消息
    log(LogLevel::INFO, "日志系统初始化，级别: %s", getLevelName(currentLogLevel).c_str());
    
    return true;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (isInitialized) {
        log(LogLevel::INFO, "日志系统关闭");
        
        if (logFileStream.is_open()) {
            logFileStream.close();
        }
        
        isInitialized = false;
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLogLevel = level;
}

LogLevel Logger::getLogLevel() {
    std::lock_guard<std::mutex> lock(logMutex);
    return currentLogLevel;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, "%s", message.c_str());
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, "%s", message.c_str());
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, "%s", message.c_str());
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, "%s", message.c_str());
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, "%s", message.c_str());
}

void Logger::log(LogLevel level, const char* format, ...) {
    if (level < currentLogLevel) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // 格式化消息
    const int bufferSize = 4096;
    char buffer[bufferSize];
    int result = vsnprintf(buffer, bufferSize, format, args);
    va_end(args);
    
    if (result >= 0 && result < bufferSize) {
        std::string message(buffer, result);
        logOutput(level, message);
    }
}

void Logger::logOutput(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timeStr = getCurrentTimeString();
    std::string levelStr = getLevelName(level);
    std::string fullMessage = timeStr + " [" + levelStr + "] " + message;
    
    // 输出到控制台
    if (enableConsoleOutput) {
        std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        out << fullMessage << std::endl;
    }
    
    // 输出到文件
    if (logFileStream.is_open()) {
        logFileStream << fullMessage << std::endl;
        logFileStream.flush();
    }
}

std::string Logger::getLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                 now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

} // namespace utils
} // namespace dlogcover 