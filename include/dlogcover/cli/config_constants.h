/**
 * @file config_constants.h
 * @brief 配置常量定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CLI_CONFIG_CONSTANTS_H
#define DLOGCOVER_CLI_CONFIG_CONSTANTS_H

namespace dlogcover {
namespace config {

/**
 * @brief 当前支持的配置文件版本
 */
constexpr const char* CURRENT_CONFIG_VERSION = "1.0";

/**
 * @brief JSON配置文件键名
 */
namespace json {
    constexpr const char* KEY_VERSION = "version";
    constexpr const char* KEY_DIRECTORY = "directory";
    constexpr const char* KEY_OUTPUT = "output";
    constexpr const char* KEY_LOG_LEVEL = "log_level";
    constexpr const char* KEY_REPORT_FORMAT = "report_format";
    constexpr const char* KEY_EXCLUDE = "exclude";
    constexpr const char* KEY_LOG_PATH = "log_path";
} // namespace json

/**
 * @brief 日志级别字符串常量
 */
namespace log {
    constexpr const char* DEBUG = "debug";
    constexpr const char* INFO = "info";
    constexpr const char* WARNING = "warning";
    constexpr const char* CRITICAL = "critical";
    constexpr const char* FATAL = "fatal";
    constexpr const char* ALL = "all";
} // namespace log

/**
 * @brief 报告格式字符串常量
 */
namespace report {
    constexpr const char* TEXT = "text";
    constexpr const char* JSON = "json";
} // namespace report

/**
 * @brief 错误消息常量
 */
namespace error {
    constexpr const char* INVALID_VERSION = "不支持的配置文件版本: ";
    constexpr const char* FILE_OPEN = "无法打开文件: ";
    constexpr const char* FILE_NOT_FOUND = "文件不存在: ";
    constexpr const char* DIRECTORY_NOT_FOUND = "目录不存在: ";
    constexpr const char* OUTPUT_DIR_NOT_FOUND = "输出目录不存在: ";
    constexpr const char* INVALID_TYPE = "字段类型不正确: ";
    constexpr const char* MISSING_FIELD = "缺少必需字段: ";
    constexpr const char* INVALID_LOG_LEVEL = "无效的日志级别: ";
    constexpr const char* INVALID_REPORT_FORMAT = "无效的报告格式: ";
    constexpr const char* INVALID_EXCLUDE = "无效的排除模式";
    constexpr const char* PARSE_ERROR = "配置文件解析错误";
} // namespace error

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CLI_CONFIG_CONSTANTS_H 