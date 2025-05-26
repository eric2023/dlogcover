/**
 * @file log_types.h
 * @brief 日志类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_LOG_IDENTIFIER_LOG_TYPES_H
#define DLOGCOVER_CORE_LOG_IDENTIFIER_LOG_TYPES_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>
#include <vector>

namespace dlogcover {
namespace core {
namespace log_identifier {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    UNKNOWN = -1,       ///< 未知级别
    DEBUG = 0,          ///< 调试级别
    INFO = 1,           ///< 信息级别
    WARNING = 2,        ///< 警告级别
    CRITICAL = 3,       ///< 严重级别
    FATAL = 4           ///< 致命级别
};

/**
 * @brief 日志类型枚举
 */
enum class LogType {
    UNKNOWN = 0,        ///< 未知类型
    QT = 1,             ///< Qt日志
    CUSTOM = 2,         ///< 自定义日志
    QT_CATEGORY = 3     ///< Qt分类日志
};

/**
 * @brief 日志调用类型枚举
 */
enum class LogCallType {
    UNKNOWN = 0,        ///< 未知调用类型
    DIRECT = 1,         ///< 直接调用
    MACRO = 2,          ///< 宏调用
    FUNCTION = 3,       ///< 函数调用
    STREAM = 4,         ///< 流式调用
    FORMAT = 5          ///< 格式化调用
};

/**
 * @brief 日志调用信息结构
 */
struct LogCallInfo {
    std::string functionName;           ///< 函数名
    LogLevel level;                     ///< 日志级别
    LogType type;                       ///< 日志类型
    LogCallType callType;               ///< 调用类型
    ast_analyzer::LocationInfo location; ///< 位置信息
    std::string message;                ///< 日志消息
    std::string category;               ///< 日志分类（Qt分类日志使用）
    std::string contextPath;            ///< 上下文路径
    std::vector<std::string> arguments; ///< 参数列表
    bool isFormatted;                   ///< 是否为格式化日志
    
    /**
     * @brief 默认构造函数
     */
    LogCallInfo() 
        : level(LogLevel::UNKNOWN)
        , type(LogType::UNKNOWN)
        , callType(LogCallType::UNKNOWN)
        , isFormatted(false) {}
};

/**
 * @brief 日志级别转字符串
 * @param level 日志级别
 * @return 字符串表示
 */
std::string toString(LogLevel level);

/**
 * @brief 字符串转日志级别
 * @param str 字符串
 * @return 日志级别
 */
LogLevel parseLogLevel(const std::string& str);

/**
 * @brief 日志类型转字符串
 * @param type 日志类型
 * @return 字符串表示
 */
std::string toString(LogType type);

/**
 * @brief 字符串转日志类型
 * @param str 字符串
 * @return 日志类型
 */
LogType parseLogType(const std::string& str);

} // namespace log_identifier
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_LOG_IDENTIFIER_LOG_TYPES_H 