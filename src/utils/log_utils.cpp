/**
 * @file log_utils.cpp
 * @brief 日志工具函数实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/string_utils.h>
#include <dlogcover/common/log_types.h>

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace dlogcover {
namespace utils {

// 静态成员变量定义 - 使用原子变量确保线程安全
std::atomic<LogLevel> Logger::currentLogLevel_{common::getDefaultLogLevel()};
std::string Logger::logFilePath_;
bool Logger::enableConsoleOutput_ = true;
std::mutex Logger::logMutex_;
std::ofstream Logger::logFileStream_;
std::atomic<bool> Logger::isInitialized_{false};
std::atomic<bool> Logger::hasBeenInitialized_{false};
std::mutex Logger::initMutex_;

bool Logger::init(const std::string& logFileName, bool consoleOutput, LogLevel level) {
    // 使用双检锁模式避免多线程初始化问题
    if (isInitialized_.load(std::memory_order_acquire)) {
        return true;
    }

    std::lock_guard<std::mutex> lock(initMutex_);
    if (isInitialized_.load(std::memory_order_relaxed)) {
        return true;
    }

    // 关闭已打开的日志文件
    if (logFileStream_.is_open()) {
        logFileStream_.close();
    }

    // 设置日志级别
    currentLogLevel_.store(level, std::memory_order_relaxed);

    // 设置控制台输出
    enableConsoleOutput_ = consoleOutput;

    // 设置日志文件路径
    logFilePath_ = logFileName;

    // 如果日志目录不存在，则创建
    std::string dirPath = FileUtils::getDirectoryName(logFilePath_);
    if (!dirPath.empty() && !FileUtils::directoryExists(dirPath)) {
        FileUtils::createDirectory(dirPath);
    }

    // 打开日志文件
    logFileStream_.open(logFilePath_, std::ios::out | std::ios::app);
    if (!logFileStream_.is_open() && !logFilePath_.empty()) {
        if (enableConsoleOutput_) {
            std::cerr << "无法打开日志文件: " << logFilePath_ << std::endl;
        }
        return false;
    }

    isInitialized_.store(true, std::memory_order_release);
    hasBeenInitialized_.store(true, std::memory_order_release);

    // 记录初始化消息 - 直接输出避免递归调用
    LogLevel currentLevel = currentLogLevel_.load(std::memory_order_relaxed);
    if (enableConsoleOutput_) {
        std::cout << getCurrentTimeString() << " [INFO] 日志系统初始化，级别: " 
                  << getLevelName(currentLevel) << std::endl;
    }
    if (logFileStream_.is_open()) {
        logFileStream_ << getCurrentTimeString() << " [INFO] 日志系统初始化，级别: " 
                       << getLevelName(currentLevel) << std::endl;
        logFileStream_.flush();
    }

    return true;
}

void Logger::shutdown() {
    if (!isInitialized_.load(std::memory_order_acquire)) {
        return;
    }

    std::lock_guard<std::mutex> lock(initMutex_);
    if (!isInitialized_.load(std::memory_order_relaxed)) {
        return;
    }

    {
        std::lock_guard<std::mutex> logLock(logMutex_);

        // 直接输出关闭消息，而不是通过log()函数
        std::string timeStr = getCurrentTimeString();
        std::string fullMessage = timeStr + " [INFO] 日志系统关闭";

        // 输出到控制台
        if (enableConsoleOutput_) {
            std::cout << fullMessage << std::endl;
        }

        // 输出到文件
        if (logFileStream_.is_open()) {
            logFileStream_ << fullMessage << std::endl;
            logFileStream_.flush();
            logFileStream_.close();
        }
    }

    isInitialized_.store(false, std::memory_order_release);
}

void Logger::setLogLevel(LogLevel level) {
    if (!common::isValidLogLevel(level)) {
        // 如果级别无效，记录警告并使用默认级别
        if (isInitialized_.load(std::memory_order_acquire)) {
            logOutput(LogLevel::WARNING, "设置的日志级别无效，使用默认级别INFO");
        }
        level = common::getDefaultLogLevel();
    }
    
    currentLogLevel_.store(level, std::memory_order_relaxed);
    
    // 记录级别变更
    if (isInitialized_.load(std::memory_order_acquire)) {
        logOutput(LogLevel::INFO, "日志级别已更改为: " + std::string(common::toString(level)));
    }
}

LogLevel Logger::getLogLevel() {
    return currentLogLevel_.load(std::memory_order_relaxed);
}

bool Logger::isInitialized() {
    return isInitialized_.load(std::memory_order_acquire);
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
    // 如果未初始化，先进行初始化
    if (!isInitialized_.load(std::memory_order_acquire)) {
        return;
    }
    
    // 快速检查日志级别，避免不必要的格式化
    LogLevel currentLevel = currentLogLevel_.load(std::memory_order_relaxed);
    if (!common::shouldLog(level, currentLevel)) {
        return;
    }

    va_list args;
    va_start(args, format);

    // 格式化消息 - 使用更安全的方法
    std::string message;

    // 先尝试一个合理的缓冲区大小
    const int initialSize = 256;
    char buffer[initialSize];
    int result = vsnprintf(buffer, initialSize, format, args);
    va_end(args);

    if (result < 0) {
        // 格式化错误
        message = "格式化日志消息失败";
    } else if (result < initialSize) {
        // 缓冲区足够大
        message = std::string(buffer, result);
    } else {
        // 需要更大的缓冲区
        va_list args2;
        va_start(args2, format);
        std::vector<char> largerBuffer(result + 1);
        vsnprintf(largerBuffer.data(), largerBuffer.size(), format, args2);
        va_end(args2);
        message = std::string(largerBuffer.data(), result);
    }

    // 输出日志
    logOutput(level, message);
}

void Logger::logOutput(LogLevel level, const std::string& message) {
    // 再次检查日志级别，避免输出低级别日志
    LogLevel currentLevel = currentLogLevel_.load(std::memory_order_relaxed);
    if (!common::shouldLog(level, currentLevel)) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex_);

    std::string timeStr = getCurrentTimeString();
    std::string levelStr = getLevelName(level);
    std::string fullMessage = timeStr + " [" + levelStr + "] " + message;

    // 输出到控制台
    if (enableConsoleOutput_) {
        std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        out << fullMessage << std::endl;
    }

    // 输出到文件
    if (logFileStream_.is_open()) {
        logFileStream_ << fullMessage << std::endl;
        logFileStream_.flush();
    }
}

std::string Logger::getLevelName(LogLevel level) {
    // 使用统一的toString函数，并转换为大写
    std::string levelStr{common::toString(level)};
    std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::toupper);
    return levelStr;
}

std::string Logger::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 更安全的时间处理，避免在多线程环境下使用不安全的 localtime 函数
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

}  // namespace utils
}  // namespace dlogcover
