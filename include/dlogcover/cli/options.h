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
#include <type_traits>

namespace dlogcover {
namespace cli {

// 使用error_types.h中定义的枚举类型，避免重复定义
using LogLevel = dlogcover::cli::LogLevel;
using ReportFormat = dlogcover::cli::ReportFormat;
using ConfigError = dlogcover::cli::ConfigError;
using ErrorResult = dlogcover::cli::ErrorResult;

/**
 * @brief 命令行选项结构
 */
struct Options {
    std::string directory;                      ///< 扫描目录路径
    std::string output_file;                    ///< 输出路径
    std::string configPath;                     ///< 配置文件路径
    std::string log_file;                       ///< 日志文件路径
    std::vector<std::string> excludePatterns;   ///< 排除模式列表
    std::vector<std::string> includePaths;      ///< 头文件搜索路径列表
    std::vector<std::string> includePatterns;   ///< 仅扫描模式列表（新增）
    LogLevel logLevel;                          ///< 日志级别
    ReportFormat reportFormat;                  ///< 报告格式
    bool showHelp;                              ///< 是否显示帮助
    bool showVersion;                           ///< 是否显示版本
    bool quiet;                                 ///< 静默模式
    bool verbose;                               ///< 详细输出模式
    
    // 性能相关选项
    bool disableParallel;                       ///< 禁用并行分析
    size_t maxThreads;                          ///< 最大线程数
    bool disableCache;                          ///< 禁用AST缓存
    size_t maxCacheSize;                        ///< 最大缓存大小
    bool disableIOOptimization;                 ///< 禁用I/O优化
    std::string mode;                           ///< 分析模式 (cpp_only/go_only/auto_detect)

    /**
     * @brief 默认构造函数
     */
    Options() 
        : logLevel(LogLevel::UNKNOWN)
        , reportFormat(ReportFormat::UNKNOWN)
        , showHelp(false)
        , showVersion(false)
        , quiet(false)
        , verbose(false)
        , disableParallel(false)
        , maxThreads(0)
        , disableCache(false)
        , maxCacheSize(100)
        , disableIOOptimization(false)
        , mode("")
    {
        // 字符串成员默认构造为空字符串 ""
    }

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
     * @brief 重置选项为默认值
     */
    void reset();

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

    /**
     * @brief 检查选项是否有效
     * @return 如果选项有效返回true，否则返回false
     */
    bool isValid() const;

    /**
     * @brief 等于运算符
     * @param other 另一个选项对象
     * @return 如果相等返回true，否则返回false
     */
    bool operator==(const Options& other) const;

    /**
     * @brief 不等于运算符
     * @param other 另一个选项对象
     * @return 如果不相等返回true，否则返回false
     */
    bool operator!=(const Options& other) const;
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
 * @brief 模板函数：支持任何可转换为字符串的类型
 * @tparam T 输入类型
 * @param value 输入值
 * @return 日志级别
 */
template<typename T>
LogLevel parseLogLevel(const T& value) {
    if constexpr (std::is_convertible_v<T, std::string>) {
        return parseLogLevel(std::string_view(static_cast<std::string>(value)));
    } else {
        return parseLogLevel(std::string_view(value.template get<std::string>()));
    }
}

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

/**
 * @brief 模板函数：支持任何可转换为字符串的类型
 * @tparam T 输入类型
 * @param value 输入值
 * @return 报告格式
 */
template<typename T>
ReportFormat parseReportFormat(const T& value) {
    if constexpr (std::is_convertible_v<T, std::string>) {
        return parseReportFormat(std::string_view(static_cast<std::string>(value)));
    } else {
        return parseReportFormat(std::string_view(value.template get<std::string>()));
    }
}

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_OPTIONS_H 