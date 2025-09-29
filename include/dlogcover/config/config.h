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
    std::vector<std::string> include_patterns; ///< 仅扫描模式列表（新增）
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
    bool show_uncovered_paths_details = false; ///< 是否显示未覆盖路径详细列表
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
    std::string mode = "cpp_only";           ///< 分析模式：cpp_only, go_only, auto_detect
    
    /// 自动检测配置
    struct AutoDetectionConfig {
        size_t sample_size = 10;             ///< 抽样文件数量
        double confidence_threshold = 0.8;   ///< 置信度阈值
    } auto_detection;
    
    bool function_coverage = true;           ///< 是否分析函数覆盖率
    bool branch_coverage = true;             ///< 是否分析分支覆盖率
    bool exception_coverage = true;          ///< 是否分析异常处理覆盖率
    bool key_path_coverage = true;           ///< 是否分析关键路径覆盖率
};

/**
 * @brief 性能配置
 */
struct PerformanceConfig {
    bool enable_parallel_analysis = true;   ///< 是否启用并行分析
    size_t max_threads = 0;                 ///< 最大线程数，0表示自动检测
    bool enable_ast_cache = true;           ///< 是否启用AST缓存
    size_t max_cache_size = 100;            ///< 最大缓存大小
    bool enable_io_optimization = true;     ///< 是否启用I/O优化
    size_t file_buffer_size = 64 * 1024;    ///< 文件缓冲区大小
    bool enable_file_preloading = true;     ///< 是否启用文件预加载
};

/**
 * @brief Go语言配置
 */
struct GoConfig {    
    /// 标准库log包配置
    struct StandardLogConfig {
        bool enabled = true;                 ///< 是否启用标准库log函数识别
        std::vector<std::string> functions = {
            "log.Print", "log.Printf", "log.Println",
            "log.Fatal", "log.Fatalf", "log.Fatalln",
            "log.Panic", "log.Panicf", "log.Panicln"
        };
    } standard_log;
    
    /// Logrus日志库配置
    struct LogrusConfig {
        bool enabled = true;                 ///< 是否启用Logrus函数识别
        std::vector<std::string> functions = {
            "logrus.Trace", "logrus.Debug", "logrus.Info", 
            "logrus.Warn", "logrus.Error", "logrus.Fatal", "logrus.Panic"
        };
        std::vector<std::string> formatted_functions = {
            "logrus.Tracef", "logrus.Debugf", "logrus.Infof",
            "logrus.Warnf", "logrus.Errorf", "logrus.Fatalf", "logrus.Panicf"
        };
        std::vector<std::string> line_functions = {
            "logrus.Traceln", "logrus.Debugln", "logrus.Infoln",
            "logrus.Warnln", "logrus.Errorln", "logrus.Fatalln", "logrus.Panicln"
        };
    } logrus;
    
    /// Zap日志库配置
    struct ZapConfig {
        bool enabled = true;                 ///< 是否启用Zap函数识别
        std::vector<std::string> logger_functions = {
            "Debug", "Info", "Warn", "Error", "DPanic", "Panic", "Fatal"
        };
        std::vector<std::string> sugared_functions = {
            "Debugf", "Debugln", "Debugw", "Infof", "Infoln", "Infow",
            "Warnf", "Warnln", "Warnw", "Errorf", "Errorln", "Errorw",
            "DPanicf", "DPanicln", "DPanicw", "Panicf", "Panicln", "Panicw",
            "Fatalf", "Fatalln", "Fatalw"
        };
    } zap;
    
    /// golib logger模块配置
    struct GolibConfig {
        bool enabled = true;                 ///< 是否启用golib logger函数识别
        std::vector<std::string> functions = {
            "log.Info", "log.Error", "log.Debug", "log.Warn"
        };
        std::vector<std::string> formatted_functions = {
            "log.Infof", "log.Errorf", "log.Debugf", "log.Warnf"
        };
    } golib;
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
    PerformanceConfig performance;           ///< 性能配置
    GoConfig go;                             ///< Go语言配置
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_CONFIG_H 