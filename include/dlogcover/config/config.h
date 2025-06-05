/**
 * @file config.h
 * @brief 配置结构定义
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
 * @brief CMake配置
 */
struct CMakeConfig {
    bool enabled;                            ///< 是否启用CMake参数自动检测
    std::string cmakeListsPath;              ///< CMakeLists.txt文件路径（空则自动查找）
    std::string targetName;                  ///< 目标名称（空则使用全局参数）
    bool useCompileCommands;                 ///< 是否使用compile_commands.json
    std::string compileCommandsPath;         ///< compile_commands.json路径
    bool verboseLogging;                     ///< 是否启用详细日志
};

/**
 * @brief 扫描配置
 */
struct ScanConfig {
    std::vector<std::string> directories;    ///< 扫描目录列表
    std::vector<std::string> excludes;       ///< 排除模式列表
    std::vector<std::string> fileTypes;      ///< 文件类型列表
    std::string includePathsStr;             ///< 包含路径字符串
    bool isQtProject;                        ///< 是否为Qt项目
    std::vector<std::string> compilerArgs;   ///< 编译器参数
    CMakeConfig cmake;                       ///< CMake配置
};

/**
 * @brief Qt日志配置
 */
struct QtLogConfig {
    bool enabled;                                    ///< 是否启用Qt日志识别
    std::vector<std::string> functions;             ///< Qt日志函数列表
    std::vector<std::string> categoryFunctions;     ///< Qt分类日志函数列表
};

/**
 * @brief 自定义日志配置
 */
struct CustomLogConfig {
    bool enabled;                                                    ///< 是否启用自定义日志识别
    std::map<std::string, std::vector<std::string>> functions;      ///< 按级别分类的自定义日志函数
};

/**
 * @brief 日志函数配置
 */
struct LogFunctionsConfig {
    QtLogConfig qt;                          ///< Qt日志配置
    CustomLogConfig custom;                  ///< 自定义日志配置
};

/**
 * @brief 分析配置
 */
struct AnalysisConfig {
    bool functionCoverage;                   ///< 是否分析函数覆盖率
    bool branchCoverage;                     ///< 是否分析分支覆盖率
    bool exceptionCoverage;                  ///< 是否分析异常处理覆盖率
    bool keyPathCoverage;                    ///< 是否分析关键路径覆盖率
};

/**
 * @brief 报告配置
 */
struct ReportConfig {
    std::string format;                      ///< 报告格式
    std::string timestampFormat;             ///< 时间戳格式
};

/**
 * @brief 总配置类
 */
struct Config {
    ScanConfig scan;                         ///< 扫描配置
    LogFunctionsConfig logFunctions;         ///< 日志函数配置
    AnalysisConfig analysis;                 ///< 分析配置
    ReportConfig report;                     ///< 报告配置
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_CONFIG_H 