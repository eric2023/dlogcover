/**
 * @file log_types.h
 * @brief 统一的日志类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_COMMON_LOG_TYPES_H
#define DLOGCOVER_COMMON_LOG_TYPES_H

#include <string>
#include <string_view>

namespace dlogcover {
namespace common {

/**
 * @brief 统一的日志级别枚举
 * 
 * 这是项目中唯一的日志级别定义，所有模块都应该使用这个枚举
 */
enum class LogLevel {
    UNKNOWN = -1,   ///< 未知级别 - 用于日志识别时无法确定级别的情况
    DEBUG = 0,      ///< 调试级别 - 详细的调试信息
    INFO = 1,       ///< 信息级别 - 一般信息
    WARNING = 2,    ///< 警告级别 - 警告信息
    ERROR = 3,      ///< 错误级别 - 错误信息
    CRITICAL = 3,   ///< 严重级别 - 与ERROR相同级别，保持兼容性
    FATAL = 4,      ///< 致命级别 - 致命错误
    ALL = 5         ///< 所有级别 - 用于CLI参数，表示输出所有级别
};

/**
 * @brief 日志级别转字符串
 * @param level 日志级别
 * @return 字符串表示
 */
std::string_view toString(LogLevel level);

/**
 * @brief 字符串转日志级别
 * @param str 字符串
 * @return 日志级别
 * @throws std::invalid_argument 如果字符串无效
 */
LogLevel parseLogLevel(std::string_view str);

/**
 * @brief 检查日志级别是否有效
 * @param level 日志级别
 * @return 有效返回true，否则返回false
 */
bool isValidLogLevel(LogLevel level);

/**
 * @brief 比较两个日志级别
 * @param level1 第一个日志级别
 * @param level2 第二个日志级别
 * @return level1 >= level2 返回true，否则返回false
 */
bool shouldLog(LogLevel level1, LogLevel level2);

/**
 * @brief 获取默认日志级别
 * @return 默认日志级别（INFO）
 */
LogLevel getDefaultLogLevel();

} // namespace common
} // namespace dlogcover

#endif // DLOGCOVER_COMMON_LOG_TYPES_H 