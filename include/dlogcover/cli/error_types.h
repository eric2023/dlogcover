/**
 * @file error_types.h
 * @brief 错误类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CLI_ERROR_TYPES_H
#define DLOGCOVER_CLI_ERROR_TYPES_H

#include <dlogcover/common/log_types.h>
#include <string>

namespace dlogcover {
namespace cli {

// 使用统一的日志级别定义
using LogLevel = dlogcover::common::LogLevel;

/**
 * @brief 报告格式枚举
 */
enum class ReportFormat {
    UNKNOWN,  // 未知或未设置
    TEXT,     // 纯文本格式
    JSON      // JSON格式
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
    ParseError = 10,                    ///< 解析错误
    EnvironmentError = 11,              ///< 环境变量错误
    UnknownOption = 12,                 ///< 未知选项
    InvalidLogPath = 13,                ///< 无效日志路径
    MISSING_VALUE = 14,                 ///< 缺少值
    InvalidArgument = 15,               ///< 无效参数
    MissingArgument = 16                ///< 缺少参数
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

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_ERROR_TYPES_H 