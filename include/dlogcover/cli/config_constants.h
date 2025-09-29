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
 * @brief 默认配置值
 */
namespace cli {
    constexpr const char* DEFAULT_DIRECTORY = ".";
    constexpr const char* DEFAULT_OUTPUT = "";
    constexpr const char* DEFAULT_CONFIG = "";

    // 命令行选项常量
    constexpr const char* OPTION_HELP_SHORT = "-h";
    constexpr const char* OPTION_HELP_LONG = "--help";
    constexpr const char* OPTION_VERSION_SHORT = "-v";
    constexpr const char* OPTION_VERSION_LONG = "--version";
    constexpr const char* OPTION_DIRECTORY_SHORT = "-d";
    constexpr const char* OPTION_DIRECTORY_LONG = "--directory";
    constexpr const char* OPTION_OUTPUT_SHORT = "-o";
    constexpr const char* OPTION_OUTPUT_LONG = "--output";
    constexpr const char* OPTION_CONFIG_SHORT = "-c";
    constexpr const char* OPTION_CONFIG_LONG = "--config";
    constexpr const char* OPTION_INCLUDE_SHORT = "-i";
    constexpr const char* OPTION_INCLUDE_LONG = "--include";
    constexpr const char* OPTION_EXCLUDE_SHORT = "-e";
    constexpr const char* OPTION_EXCLUDE_LONG = "--exclude";
    constexpr const char* OPTION_LOG_LEVEL_SHORT = "-l";
    constexpr const char* OPTION_LOG_LEVEL_LONG = "--log-level";
    constexpr const char* OPTION_FORMAT_SHORT = "-f";
    constexpr const char* OPTION_FORMAT_LONG = "--format";
    constexpr const char* OPTION_LOG_PATH_SHORT = "-p";
    constexpr const char* OPTION_LOG_PATH_LONG = "--log-path";
    constexpr const char* OPTION_INCLUDE_PATH_SHORT = "-I";
    constexpr const char* OPTION_INCLUDE_PATH_LONG = "--include-path";
    constexpr const char* OPTION_QUIET_SHORT = "-q";
    constexpr const char* OPTION_QUIET_LONG = "--quiet";
    constexpr const char* OPTION_VERBOSE_LONG = "--verbose";
    constexpr const char* OPTION_MAX_THREADS_LONG = "--max-threads";
    constexpr const char* OPTION_DISABLE_PARALLEL_LONG = "--disable-parallel";
    constexpr const char* OPTION_DISABLE_CACHE_LONG = "--disable-cache";
    constexpr const char* OPTION_MAX_CACHE_SIZE_LONG = "--max-cache-size";
    constexpr const char* OPTION_DISABLE_IO_OPT_LONG = "--disable-io-opt";
    constexpr const char* OPTION_MODE_SHORT = "-m";
    constexpr const char* OPTION_MODE_LONG = "--mode";
} // namespace cli

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
    constexpr const char* UNKNOWN_OPTION = "未知选项: ";
    constexpr const char* INVALID_LOG_PATH = "无效的日志路径: ";
} // namespace error

/**
 * @brief 环境变量常量
 */
namespace env {
    constexpr const char* DIRECTORY = "DLOGCOVER_DIRECTORY";
    constexpr const char* OUTPUT = "DLOGCOVER_OUTPUT";
    constexpr const char* CONFIG = "DLOGCOVER_CONFIG";
    constexpr const char* LOG_PATH = "DLOGCOVER_LOG_PATH";
    constexpr const char* LOG_LEVEL = "DLOGCOVER_LOG_LEVEL";
    constexpr const char* REPORT_FORMAT = "DLOGCOVER_REPORT_FORMAT";
    constexpr const char* EXCLUDE = "DLOGCOVER_EXCLUDE";
} // namespace env

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CLI_CONFIG_CONSTANTS_H