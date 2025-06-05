/**
 * @file log_utils.h
 * @brief 日志工具函数
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_LOG_UTILS_H
#define DLOGCOVER_UTILS_LOG_UTILS_H

#include <fstream>
#include <mutex>
#include <string>
#include <atomic>

namespace dlogcover {
namespace utils {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    DEBUG = 0,      ///< 调试级别
    INFO = 1,       ///< 信息级别
    WARNING = 2,    ///< 警告级别
    ERROR = 3,      ///< 错误级别
    FATAL = 4,      ///< 致命级别
    CUSTOM = 5      ///< 自定义级别
};

/**
 * @brief 日志记录器类
 * 
 * 提供线程安全的日志记录功能，支持文件和控制台输出
 */
class Logger {
public:
    /**
     * @brief 初始化日志系统
     * @param logFileName 日志文件名
     * @param consoleOutput 是否输出到控制台
     * @param level 日志级别
     * @return 初始化成功返回true，否则返回false
     */
    static bool init(const std::string& logFileName, bool consoleOutput = true, LogLevel level = LogLevel::INFO);

    /**
     * @brief 关闭日志系统
     */
    static void shutdown();

    /**
     * @brief 设置日志级别
     * @param level 日志级别
     */
    static void setLogLevel(LogLevel level);

    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    static LogLevel getLogLevel();

    /**
     * @brief 检查日志系统是否已初始化
     * @return 已初始化返回true，否则返回false
     */
    static bool isInitialized();

    /**
     * @brief 记录调试日志
     * @param message 日志消息
     */
    static void debug(const std::string& message);

    /**
     * @brief 记录信息日志
     * @param message 日志消息
     */
    static void info(const std::string& message);

    /**
     * @brief 记录警告日志
     * @param message 日志消息
     */
    static void warning(const std::string& message);

    /**
     * @brief 记录错误日志
     * @param message 日志消息
     */
    static void error(const std::string& message);

    /**
     * @brief 记录致命错误日志
     * @param message 日志消息
     */
    static void fatal(const std::string& message);

    /**
     * @brief 记录格式化日志
     * @param level 日志级别
     * @param format 格式字符串
     * @param ... 格式参数
     */
    static void log(LogLevel level, const char* format, ...);

private:
    static LogLevel currentLogLevel_;           ///< 当前日志级别
    static std::string logFilePath_;            ///< 日志文件路径
    static bool enableConsoleOutput_;           ///< 是否启用控制台输出
    static std::mutex logMutex_;                ///< 日志互斥锁
    static std::ofstream logFileStream_;        ///< 日志文件流
    static std::atomic<bool> isInitialized_;    ///< 是否已初始化
    static std::mutex initMutex_;               ///< 初始化互斥锁

    /**
     * @brief 输出日志到文件和控制台
     * @param level 日志级别
     * @param message 日志消息
     */
    static void logOutput(LogLevel level, const std::string& message);

    /**
     * @brief 获取当前时间字符串
     * @return 时间字符串
     */
    static std::string getCurrentTimeString();

    /**
     * @brief 获取日志级别名称
     * @param level 日志级别
     * @return 级别名称
     */
    static std::string getLevelName(LogLevel level);
};

} // namespace utils
} // namespace dlogcover

// 日志宏定义，方便使用
#define LOG_DEBUG(msg) dlogcover::utils::Logger::debug(msg)
#define LOG_INFO(msg) dlogcover::utils::Logger::info(msg)
#define LOG_WARNING(msg) dlogcover::utils::Logger::warning(msg)
#define LOG_ERROR(msg) dlogcover::utils::Logger::error(msg)
#define LOG_FATAL(msg) dlogcover::utils::Logger::fatal(msg)

#define LOG_DEBUG_FMT(format, ...) dlogcover::utils::Logger::log(dlogcover::utils::LogLevel::DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO_FMT(format, ...) dlogcover::utils::Logger::log(dlogcover::utils::LogLevel::INFO, format, ##__VA_ARGS__)
#define LOG_WARNING_FMT(format, ...) dlogcover::utils::Logger::log(dlogcover::utils::LogLevel::WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR_FMT(format, ...) dlogcover::utils::Logger::log(dlogcover::utils::LogLevel::ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL_FMT(format, ...) dlogcover::utils::Logger::log(dlogcover::utils::LogLevel::FATAL, format, ##__VA_ARGS__)

#endif // DLOGCOVER_UTILS_LOG_UTILS_H 