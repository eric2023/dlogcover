/**
 * @file error_types.h
 * @brief 错误类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CLI_ERROR_TYPES_H
#define DLOGCOVER_CLI_ERROR_TYPES_H

#include <string>

namespace dlogcover {
namespace cli {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    DEBUG = 0,      ///< 调试级别
    INFO = 1,       ///< 信息级别
    WARNING = 2,    ///< 警告级别
    CRITICAL = 3,   ///< 严重级别
    FATAL = 4,      ///< 致命级别
    ALL = 5         ///< 所有级别
};

/**
 * @brief 报告格式枚举
 */
enum class ReportFormat {
    TEXT = 0,       ///< 文本格式
    JSON = 1        ///< JSON格式
};

/**
 * @brief 配置错误类型枚举
 */
enum class ConfigError {
    None = 0,                           ///< 无错误
    FileNotFound = 1,                   ///< 文件未找到
    DirectoryNotFound = 2,              ///< 目录未找到
    OutputDirectoryNotFound = 3,        ///< 输出目录未找到
    InvalidVersion = 4,                 ///< 无效版本
    InvalidType = 5,                    ///< 无效类型
    MissingField = 6,                   ///< 缺少字段
    InvalidLogLevel = 7,                ///< 无效日志级别
    InvalidReportFormat = 8,            ///< 无效报告格式
    InvalidExcludePattern = 9,          ///< 无效排除模式
    ParseError = 10                     ///< 解析错误
};

/**
 * @brief 错误结果类
 * 用于表示操作的错误状态和错误信息
 */
class ErrorResult {
public:
    /**
     * @brief 默认构造函数，表示无错误状态
     */
    ErrorResult() : errorCode_(ConfigError::None) {}

    /**
     * @brief 构造函数
     * @param error 错误代码
     * @param message 错误消息
     */
    ErrorResult(ConfigError error, const std::string& message)
        : errorCode_(error), errorMessage_(message) {}

    /**
     * @brief 检查是否有错误
     * @return 如果有错误返回true，否则返回false
     */
    bool hasError() const {
        return errorCode_ != ConfigError::None;
    }

    /**
     * @brief 获取错误代码
     * @return 错误代码
     */
    ConfigError error() const {
        return errorCode_;
    }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     */
    const std::string& message() const {
        return errorMessage_;
    }

    /**
     * @brief 获取错误消息（别名）
     * @return 错误消息
     */
    const std::string& errorMessage() const {
        return errorMessage_;
    }

private:
    ConfigError errorCode_;     ///< 错误代码
    std::string errorMessage_;  ///< 错误消息
};

/**
 * @brief 日志级别转字符串
 * @param level 日志级别
 * @return 对应的字符串表示
 */
std::string_view toString(LogLevel level);

/**
 * @brief 字符串转日志级别
 * @param str 字符串
 * @return 对应的日志级别
 * @throws std::invalid_argument 如果字符串无效
 */
LogLevel parseLogLevel(std::string_view str);

/**
 * @brief 报告格式转字符串
 * @param format 报告格式
 * @return 对应的字符串表示
 */
std::string_view toString(ReportFormat format);

/**
 * @brief 字符串转报告格式
 * @param str 字符串
 * @return 对应的报告格式
 * @throws std::invalid_argument 如果字符串无效
 */
ReportFormat parseReportFormat(std::string_view str);

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_ERROR_TYPES_H 