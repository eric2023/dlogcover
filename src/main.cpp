/**
 * @file main.cpp
 * @brief 程序入口
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/reporter/reporter.h>
#include <dlogcover/reporter/reporter_factory.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>

// 版本信息常量
#define DLOGCOVER_VERSION_MAJOR 1
#define DLOGCOVER_VERSION_MINOR 0
#define DLOGCOVER_VERSION_PATCH 0
#define DLOGCOVER_VERSION_STR "1.0.0"

using namespace dlogcover;

// RAII日志管理类，确保日志系统正确关闭
class LogManager {
public:
    LogManager(const std::string& logFileName, bool console, utils::LogLevel level) {
        utils::Logger::init(logFileName, console, level);
        LOG_INFO_FMT("DLogCover v%s 启动", DLOGCOVER_VERSION_STR);
    }

    ~LogManager() {
        LOG_INFO("DLogCover 关闭日志系统");
        utils::Logger::shutdown();
    }
};

// 计时工具类
class Timer {
public:
    Timer(const std::string& operationName)
        : operationName_(operationName), start_(std::chrono::high_resolution_clock::now()) {
        LOG_DEBUG_FMT("开始执行: %s", operationName_.c_str());
    }

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
        LOG_INFO_FMT("%s 执行完成, 耗时: %.2f 秒", operationName_.c_str(), duration / 1000.0);
    }

private:
    std::string operationName_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// 解析命令行并获取选项
bool parseCommandLine(int argc, char* argv[], cli::CommandLineParser& parser) {
    Timer timer("命令行解析");

    auto result = parser.parse(argc, argv);
    
    // 首先检查是否是帮助或版本请求（无论是否有错误）
    if (parser.isHelpOrVersionRequest()) {
        if (parser.isHelpRequest()) {
            parser.showHelp();
        } else if (parser.isVersionRequest()) {
            parser.showVersion();
        }
        return false;  // 帮助或版本请求，正常退出
    }
    
    // 然后检查其他错误
    if (result.hasError()) {
        // 显示错误信息和帮助提示
        std::cerr << "错误: " << result.errorMessage() << std::endl;
        std::cerr << "使用 --help 查看使用说明" << std::endl;
        LOG_ERROR("命令行参数解析失败");
        return false;  // 其他解析错误
    }
    return true;
}

// 加载和验证配置
bool loadAndValidateConfig(const cli::Options& options, config::ConfigManager& configManager) {
    Timer timer("配置加载和验证");

    // loadConfig现在即使配置文件不存在也会返回true，因为会使用默认配置
    if (!configManager.loadConfig(options.configPath)) {
        // 这里应该不会再进来，但保留以防万一
        LOG_ERROR("无法加载配置文件或默认配置: " + options.configPath);
        return false;
    }

    // 与命令行选项合并
    configManager.mergeWithCommandLineOptions(options);

    // 验证配置
    if (!configManager.validateConfig()) {
        LOG_ERROR("配置验证失败");
        return false;
    }

    return true;
}

// 收集源文件
bool collectSourceFiles(const config::Config& config, source_manager::SourceManager& sourceManager) {
    Timer timer("源文件收集");

    LOG_INFO("开始收集源文件");
    if (!sourceManager.collectSourceFiles()) {
        LOG_ERROR("源文件收集失败");
        return false;
    }
    LOG_INFO_FMT("共收集到%lu个源文件", sourceManager.getSourceFileCount());
    return true;
}

// 执行AST分析
bool performASTAnalysis(const config::Config& config, source_manager::SourceManager& sourceManager,
                        core::ast_analyzer::ASTAnalyzer& astAnalyzer) {
    Timer timer("AST分析");

    LOG_INFO("开始AST分析");
    if (!astAnalyzer.analyzeAll()) {
        LOG_ERROR("AST分析失败");
        return false;
    }
    LOG_INFO("AST分析完成");
    return true;
}

// 识别日志调用
bool identifyLogCalls(const config::Config& config, core::ast_analyzer::ASTAnalyzer& astAnalyzer,
                      core::log_identifier::LogIdentifier& logIdentifier) {
    Timer timer("日志调用识别");

    LOG_INFO("开始识别日志调用");
    if (!logIdentifier.identifyLogCalls()) {
        LOG_ERROR("日志调用识别失败");
        return false;
    }
    LOG_INFO("日志调用识别完成");
    return true;
}

// 计算覆盖率
bool calculateCoverage(const config::Config& config, core::ast_analyzer::ASTAnalyzer& astAnalyzer,
                       core::log_identifier::LogIdentifier& logIdentifier,
                       core::coverage::CoverageCalculator& coverageCalculator) {
    Timer timer("覆盖率计算");

    LOG_INFO("开始计算覆盖率");
    if (!coverageCalculator.calculate()) {
        LOG_ERROR("覆盖率计算失败");
        return false;
    }
    LOG_INFO("覆盖率计算完成");
    return true;
}

// 生成报告
bool generateReport(const config::Config& config, const cli::Options& options,
                    core::coverage::CoverageCalculator& coverageCalculator) {
    Timer timer("报告生成");

    LOG_INFO("开始生成报告");
    reporter::Reporter reporter(config, coverageCalculator);

    // 定义进度回调函数
    int lastProgressPercentage = -1;
    reporter::ProgressCallback progressCallback = [&lastProgressPercentage](float progress,
                                                                            const std::string& message) {
        // 计算整数百分比
        int progressPercentage = static_cast<int>(progress * 100);

        // 只在进度变化显著时输出INFO级别日志
        if (progressPercentage % 10 == 0 && progressPercentage != lastProgressPercentage) {
            LOG_INFO_FMT("报告生成进度: %d%% - %s", progressPercentage, message.c_str());
            lastProgressPercentage = progressPercentage;
        }

        // 更详细的进度信息记录在DEBUG级别
        LOG_DEBUG_FMT("报告生成进度: %.1f%% - %s", progress * 100, message.c_str());
    };

    // 根据命令行选项，使用相应的报告格式
    reporter::ReportFormat format;
    if (options.reportFormat == cli::ReportFormat::TEXT) {
        format = reporter::ReportFormat::TEXT;
    } else if (options.reportFormat == cli::ReportFormat::JSON) {
        format = reporter::ReportFormat::JSON;
    } else {
        // 默认使用文本格式
        format = reporter::ReportFormat::TEXT;
    }

    // 使用新的接口生成报告
    auto result = reporter.generateReport(options.outputPath, format, progressCallback);
    if (result.hasError()) {
        LOG_ERROR_FMT("报告生成失败: %s", result.errorMessage().c_str());
        return false;
    }
    LOG_INFO("报告生成完成: " + options.outputPath);
    return true;
}

// 生成日志文件名
std::string generateLogFileName(const cli::Options& options) {
    // 如果配置中指定了日志文件名，则使用配置中的名称
    if (!options.logPath.empty()) {
        return options.logPath;
    }

    // 否则使用默认名称加时间戳
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "dlogcover_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";
    return ss.str();
}

int main(int argc, char* argv[]) {
    // 记录程序开始时间
    auto programStart = std::chrono::high_resolution_clock::now();

    try {
        // 解析命令行参数
        cli::CommandLineParser commandLineParser;
        if (!parseCommandLine(argc, argv, commandLineParser)) {
            // 正常退出（帮助或版本信息）或解析错误
            return commandLineParser.isHelpOrVersionRequest() ? 0 : 1;
        }

        // 获取命令行选项
        const cli::Options& options = commandLineParser.getOptions();

        // 根据配置生成日志文件名并初始化日志系统
        std::string logFileName = generateLogFileName(options);
        LogManager logManager(logFileName, true, utils::LogLevel::CUSTOM);

        // 加载配置文件
        config::ConfigManager configManager;
        if (!loadAndValidateConfig(options, configManager)) {
            return 1;
        }

        const config::Config& config = configManager.getConfig();

        // 创建源文件管理器
        source_manager::SourceManager sourceManager(config);
        if (!collectSourceFiles(config, sourceManager)) {
            return 1;
        }

        // 创建AST分析器
        core::ast_analyzer::ASTAnalyzer astAnalyzer(config, sourceManager);
        if (!performASTAnalysis(config, sourceManager, astAnalyzer)) {
            return 1;
        }

        // 创建日志识别器
        core::log_identifier::LogIdentifier logIdentifier(config, astAnalyzer);
        if (!identifyLogCalls(config, astAnalyzer, logIdentifier)) {
            return 1;
        }

        // 创建覆盖率计算器
        core::coverage::CoverageCalculator coverageCalculator(config, astAnalyzer, logIdentifier);
        if (!calculateCoverage(config, astAnalyzer, logIdentifier, coverageCalculator)) {
            return 1;
        }

        // 生成报告
        if (!generateReport(config, options, coverageCalculator)) {
            return 1;
        }

        // 计算并记录总执行时间
        auto programEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(programEnd - programStart).count();
        LOG_INFO_FMT("DLogCover执行完成，总耗时: %.2f 秒", duration / 1000.0);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        LOG_ERROR_FMT("标准异常: %s", e.what());
        return 1;
    } catch (...) {
        std::cerr << "未知错误" << std::endl;
        LOG_ERROR("未知异常");
        return 1;
    }
}
