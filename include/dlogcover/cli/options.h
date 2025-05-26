/**
 * @file options.h
 * @brief 命令行选项定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CLI_OPTIONS_H
#define DLOGCOVER_CLI_OPTIONS_H

#include <dlogcover/cli/error_types.h>
#include <string>
#include <string_view>
#include <vector>

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
    None = 0,                       ///< 无错误
    DirectoryNotFound,              ///< 目录不存在
    FileNotFound,                   ///< 文件不存在
    OutputDirectoryNotFound,        ///< 输出目录不存在
    InvalidExcludePattern,          ///< 无效的排除模式
    InvalidLogLevel,                ///< 无效的日志级别
    InvalidReportFormat,            ///< 无效的报告格式
    InvalidPath,                    ///< 无效的路径
    JsonParseError                  ///< JSON解析错误
};

/**
 * @brief 错误结果类
 */
class ErrorResult {
public:
    /**
     * @brief 默认构造函数（无错误）
     */
    ErrorResult() : error_(ConfigError::None) {}

    /**
     * @brief 构造函数（有错误）
     * @param error 错误类型
     * @param message 错误消息
     */
    ErrorResult(ConfigError error, const std::string& message) 
        : error_(error), message_(message) {}

    /**
     * @brief 检查是否有错误
     * @return 如果有错误返回true，否则返回false
     */
    bool hasError() const { return error_ != ConfigError::None; }

    /**
     * @brief 获取错误类型
     * @return 错误类型
     */
    ConfigError error() const { return error_; }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     */
    const std::string& message() const { return message_; }

private:
    ConfigError error_;     ///< 错误类型
    std::string message_;   ///< 错误消息
};

/**
 * @brief 命令行选项结构
 */
struct Options {
    std::string directoryPath;                  ///< 扫描目录路径
    std::string outputPath;                     ///< 输出路径
    std::string configPath;                     ///< 配置文件路径
    std::vector<std::string> excludePatterns;   ///< 排除模式列表
    LogLevel logLevel;                          ///< 日志级别
    ReportFormat reportFormat;                  ///< 报告格式
    bool showHelp;                              ///< 是否显示帮助
    bool showVersion;                           ///< 是否显示版本

    /**
     * @brief 默认构造函数
     */
    Options() 
        : logLevel(LogLevel::ALL)
        , reportFormat(ReportFormat::TEXT)
        , showHelp(false)
        , showVersion(false) {}

    /**
     * @brief 验证选项有效性
     * @return 验证结果
     */
    ErrorResult validate() const;

    /**
     * @brief 转换为字符串表示
     * @return 字符串表示
     */
    std::string toString() const;

    /**
     * @brief 从JSON字符串解析选项
     * @param json JSON字符串
     * @return 解析结果
     */
    ErrorResult fromJson(std::string_view json);

    /**
     * @brief 转换为JSON字符串
     * @return JSON字符串
     */
    std::string toJson() const;
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
 */
LogLevel parseLogLevel(std::string_view str);

/**
 * @brief 报告格式转字符串
 * @param format 报告格式
 * @return 字符串表示
 */
std::string_view toString(ReportFormat format);

/**
 * @brief 字符串转报告格式
 * @param str 字符串
 * @return 报告格式
 */
ReportFormat parseReportFormat(std::string_view str);

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_OPTIONS_H 