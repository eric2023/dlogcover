/**
 * @file config.h
 * @brief 配置结构定义 - 基于compile_commands.json的简化配置
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CONFIG_CONFIG_H
#define DLOGCOVER_CONFIG_CONFIG_H

#include <string>
#include <vector>
#include <map>

namespace dlogcover {
namespace config {

/**
 * @brief 项目配置
 */
struct ProjectConfig {
    std::string name;                        ///< 项目名称
    std::string directory;                   ///< 项目根目录
    std::string build_directory;             ///< 构建目录
};

/**
 * @brief 扫描配置
 */
struct ScanConfig {
    std::vector<std::string> directories;    ///< 扫描目录列表（相对于项目根目录）
    std::vector<std::string> file_extensions; ///< 文件扩展名列表
    std::vector<std::string> exclude_patterns; ///< 排除模式列表
};

/**
 * @brief compile_commands.json配置
 */
struct CompileCommandsConfig {
    std::string path;                        ///< compile_commands.json文件路径
    bool auto_generate;                      ///< 是否自动生成
    std::vector<std::string> cmake_args;     ///< CMake参数
};

/**
 * @brief 输出配置
 */
struct OutputConfig {
    std::string report_file;                 ///< 报告文件名
    std::string log_file;                    ///< 日志文件名
    std::string log_level;                   ///< 日志级别
};

/**
 * @brief 日志函数配置
 */
struct LogFunctionsConfig {
    // Qt日志函数
    struct {
        bool enabled = true;
        std::vector<std::string> functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};
        std::vector<std::string> category_functions = {"qCDebug", "qCInfo", "qCWarning", "qCCritical"};
    } qt;
    
    // 自定义日志函数
    struct {
        bool enabled = true;
        std::map<std::string, std::vector<std::string>> functions = {
            {"debug", {"logDebug", "LOG_DEBUG", "LOG_DEBUG_FMT"}},
            {"info", {"logInfo", "LOG_INFO", "LOG_INFO_FMT"}},
            {"warning", {"logWarning", "LOG_WARNING", "LOG_WARNING_FMT"}},
            {"error", {"logError", "LOG_ERROR", "LOG_ERROR_FMT"}},
            {"fatal", {"logFatal", "LOG_FATAL", "LOG_FATAL_FMT"}}
        };
    } custom;
};

/**
 * @brief 分析配置
 */
struct AnalysisConfig {
    bool function_coverage = true;           ///< 是否分析函数覆盖率
    bool branch_coverage = true;             ///< 是否分析分支覆盖率
    bool exception_coverage = true;          ///< 是否分析异常处理覆盖率
    bool key_path_coverage = true;           ///< 是否分析关键路径覆盖率
};

/**
 * @brief 总配置类
 */
struct Config {
    ProjectConfig project;                   ///< 项目配置
    ScanConfig scan;                         ///< 扫描配置
    CompileCommandsConfig compile_commands;  ///< 编译命令配置
    OutputConfig output;                     ///< 输出配置
    LogFunctionsConfig log_functions;        ///< 日志函数配置
    AnalysisConfig analysis;                 ///< 分析配置
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_CONFIG_H 