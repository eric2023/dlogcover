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
#include <dlogcover/common/log_types.h>

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



// 计时工具类
class Timer {
public:
    Timer(const std::string& operationName)
        : operationName_(operationName), start_(std::chrono::high_resolution_clock::now()) {
        LOG_DEBUG_FMT("开始执行: %s", operationName_.c_str());
    }

    ~Timer() {
        // 只有在日志系统已初始化时才输出日志，避免在shutdown后重新初始化
        if (utils::Logger::isInitialized()) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
            LOG_INFO_FMT("%s 执行完成, 耗时: %.2f 秒", operationName_.c_str(), duration / 1000.0);
        }
    }

private:
    std::string operationName_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// 解析命令行并获取选项
// 命令行解析结果枚举
enum class ParseResult {
    SUCCESS,        // 解析成功，继续执行
    HELP_VERSION,   // 帮助或版本请求，正常退出
    ERROR          // 解析错误，错误退出
};

ParseResult parseCommandLine(int argc, char* argv[], cli::CommandLineParser& parser) {
    try {
        auto result = parser.parse(argc, argv);

        // 检查是否是帮助或版本请求
        // parse() 函数内部已经处理了信息的显示
        if (parser.isHelpOrVersionRequest()) {
            return ParseResult::HELP_VERSION;
        }

        // 检查其他解析错误
        if (result.hasError()) {
            std::cerr << "参数解析错误: " << result.message() << std::endl << std::endl;
            parser.showHelp();
            return ParseResult::ERROR;
        }
    } catch (const std::exception& e) {
        std::cerr << "参数解析时发生未知错误: " << e.what() << std::endl;
        parser.showHelp();
        return ParseResult::ERROR;
    }

    return ParseResult::SUCCESS;
}

// 加载和验证配置
bool loadAndValidateConfig(const cli::Options& options, config::ConfigManager& configManager) {
    Timer timer("配置加载");

    bool configLoaded = false;
    if (!options.configPath.empty()) {
        configLoaded = configManager.loadConfig(options.configPath);
    } else {
        // 尝试在当前目录查找dlogcover.json
        if (std::filesystem::exists("dlogcover.json")) {
            configLoaded = configManager.loadConfig("dlogcover.json");
        }
    }

    if (!configLoaded) {
        LOG_INFO("未找到配置文件，使用默认配置");
        std::string projectDir = options.directory.empty() 
                               ? std::filesystem::current_path().string() 
                               : options.directory;
        if (!configManager.initializeDefault(projectDir)) {
            LOG_ERROR("默认配置初始化失败");
            return false;
        }
    }

    // 合并命令行选项
    configManager.mergeWithCommandLineOptions(options);

    // 验证最终配置
    if (!configManager.validateConfig()) {
        LOG_ERROR("配置验证失败");
        return false;
    }

    LOG_INFO("配置加载和验证成功");
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

// 准备编译命令
bool prepareCompileCommands(const config::Config& config, config::ConfigManager& configManager) {
    Timer timer("编译命令准备");
    
    LOG_INFO("开始准备编译命令数据库");
    
    // 获取 CompileCommandsManager 实例
    auto& compileManager = configManager.getCompileCommandsManager();
    
    // 确定项目目录
    std::string projectDir = config.project.directory;
    if (projectDir.empty()) {
        projectDir = std::filesystem::current_path().string();
        LOG_INFO_FMT("项目目录未设置，使用当前工作目录: %s", projectDir.c_str());
    }
    
    // 确定构建目录
    std::string buildDir = config.project.build_directory;
    if (buildDir.empty()) {
        buildDir = projectDir + "/build";
        LOG_INFO_FMT("构建目录未设置，使用默认值: %s", buildDir.c_str());
    }
    
    // 创建构建目录
    try {
        std::filesystem::create_directories(buildDir);
        LOG_DEBUG_FMT("确保构建目录存在: %s", buildDir.c_str());
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("无法创建构建目录: %s, 错误: %s", buildDir.c_str(), e.what());
        return false;
    }
    
    // 检查是否需要生成 compile_commands.json
    std::string compileCommandsPath = config.compile_commands.path;
    bool needGenerate = config.compile_commands.auto_generate;
    
    if (needGenerate) {
        // 检查现有文件是否有效
        if (compileManager.isCompileCommandsValid(compileCommandsPath)) {
            LOG_INFO("发现有效的 compile_commands.json，跳过生成");
            needGenerate = false;
        }
    }
    
    if (needGenerate) {
        LOG_INFO("开始生成 compile_commands.json");
        if (!compileManager.generateCompileCommands(projectDir, buildDir, config.compile_commands.cmake_args)) {
            LOG_WARNING_FMT("生成 compile_commands.json 失败: %s", compileManager.getError().c_str());
            LOG_INFO("将使用默认编译参数进行分析");
        } else {
            LOG_INFO("成功生成 compile_commands.json");
        }
    }
    
    // 解析 compile_commands.json
    if (std::filesystem::exists(compileCommandsPath)) {
        if (!compileManager.parseCompileCommands(compileCommandsPath)) {
            LOG_WARNING_FMT("解析 compile_commands.json 失败: %s", compileManager.getError().c_str());
            LOG_INFO("将使用默认编译参数进行分析");
        } else {
            LOG_INFO_FMT("成功解析 compile_commands.json，包含 %zu 个文件", 
                        compileManager.getAllFiles().size());
        }
    } else {
        LOG_INFO("未找到compile_commands.json，将使用默认编译参数");
    }
    
    LOG_INFO("编译命令准备完成");
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
                    auto result = reporter.generateReport(options.output_file, format, progressCallback);
    if (result.hasError()) {
        LOG_ERROR_FMT("报告生成失败: %s", result.errorMessage().c_str());
        return false;
    }
                    LOG_INFO("报告生成完成: " + options.output_file);
    return true;
}

std::string generateLogFileName(const cli::Options& options) {
    // 如果配置中指定了日志文件名，则使用配置中的名称
    if (!options.log_file.empty()) {
        return options.log_file;
    }

    // 否则使用默认名称
    return "dlogcover.log";
}

// 初始化日志系统
bool initializeLogging(const config::Config& config) {
    Timer timer("日志系统初始化");
    
    // 使用 try-catch 块来处理无效的日志级别字符串
    common::LogLevel logLevel;
    try {
        logLevel = common::parseLogLevel(config.output.log_level);
    } catch (const std::invalid_argument& e) {
        // 如果配置的日志级别无效，则记录错误并使用默认级别
        // 注意：此时日志系统尚未初始化，因此使用std::cerr
        std::cerr << "警告: 配置的日志级别无效 '" << config.output.log_level 
                  << "', 将使用默认级别 'INFO'. 错误: " << e.what() << std::endl;
        logLevel = common::getDefaultLogLevel();
    }
    
    // 初始化日志系统
    utils::Logger::init(config.output.log_file, true, logLevel);
    
    LOG_INFO_FMT("DLogCover v%s 启动", DLOGCOVER_VERSION_STR);
    
    return true;
}

int main(int argc, char* argv[]) {
    // 记录程序开始时间
    auto programStart = std::chrono::high_resolution_clock::now();

    try {
        // 1. 解析命令行，不加载任何配置，仅填充 Options
        cli::CommandLineParser commandLineParser;
        cli::Options options;
        auto parseResult = commandLineParser.parse(argc, argv);
        
        if (commandLineParser.isHelpOrVersionRequest()) {
            return 0;
        }

        if (parseResult.hasError()) {
            std::cerr << "参数解析错误: " << parseResult.message() << std::endl << std::endl;
            commandLineParser.showHelp();
            return 1;
        }

        // 2. 初始化 ConfigManager，加载内置默认配置
        config::ConfigManager configManager;

        // 3. 加载配置文件（如果指定）
        const auto& parsedOptions = commandLineParser.getOptions();
        if (!parsedOptions.configPath.empty()) {
            if (!configManager.loadConfig(parsedOptions.configPath)) {
                std::cerr << "错误: 无法加载配置文件: " << parsedOptions.configPath << " - " << configManager.getError() << std::endl;
                return 1;
            }
        } else {
             // 尝试在当前目录查找dlogcover.json
            if (std::filesystem::exists("dlogcover.json")) {
                LOG_INFO("在当前目录找到 dlogcover.json，将加载它。");
                if (!configManager.loadConfig("dlogcover.json")) {
                     std::cerr << "错误: 无法加载 dlogcover.json - " << configManager.getError() << std::endl;
                     return 1;
                }
            }
        }
        
        // 4. 合并命令行参数，覆盖默认或文件配置
        configManager.mergeWithCommandLineOptions(parsedOptions);
        
        // 获取最终合并后的配置
        const config::Config& config = configManager.getConfig();

        // 5. 初始化日志系统
        if (!initializeLogging(config)) {
            std::cerr << "日志系统初始化失败" << std::endl;
            return 1;
        }
        
        // 记录命令行参数到日志文件
        commandLineParser.logParsedOptions();

        // 6. 验证最终配置
        if (!configManager.validateConfig()) {
            LOG_ERROR("最终配置验证失败");
            return 1;
        }

        int exitCode = 0;
        {
            // 创建源文件管理器
            source_manager::SourceManager sourceManager(config);
            if (!collectSourceFiles(config, sourceManager)) {
                exitCode = 1;
            } else {
                // 新增：准备编译命令数据库
                if (!prepareCompileCommands(config, configManager)) {
                    LOG_WARNING("编译命令准备失败，将使用默认参数");
                }
                
                // 创建AST分析器
                core::ast_analyzer::ASTAnalyzer astAnalyzer(config, sourceManager, configManager);

                // 执行AST分析
                if (!performASTAnalysis(config, sourceManager, astAnalyzer)) {
                    exitCode = 1;
                } else {
                    // 创建日志识别器和覆盖率计算器
                    core::log_identifier::LogIdentifier logIdentifier(config, astAnalyzer);
                    core::coverage::CoverageCalculator coverageCalculator(config, astAnalyzer, logIdentifier);
                    coverageCalculator.calculate();

                    // 创建报告器并生成报告
                    reporter::Reporter reporter(config, coverageCalculator);
                    reporter::ReportFormat format = parsedOptions.reportFormat == cli::ReportFormat::JSON 
                                                  ? reporter::ReportFormat::JSON 
                                                  : reporter::ReportFormat::TEXT;

                    auto reportResult = reporter.generateReport(config.output.report_file, format);
                    if (reportResult.hasError()) {
                        LOG_ERROR_FMT("报告生成失败: %s", reportResult.errorMessage().c_str());
                        exitCode = 1;
                    } else {
                        LOG_INFO("报告生成完成: " + config.output.report_file);
                    }
                }
            }
        }
        
        // 在程序结束前明确关闭日志，确保所有缓冲区的日志都已写入文件
        utils::Logger::shutdown();
        
        auto programEnd = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(programEnd - programStart).count();
        std::cout << "总执行时间: " << std::fixed << std::setprecision(2) << totalDuration / 1000.0 << " 秒" << std::endl;

        return exitCode;
    } catch (const std::exception& e) {
        // 在程序结束前明确关闭日志
        utils::Logger::shutdown();
        std::cerr << "程序意外终止: " << e.what() << std::endl;
        return 1;
    }
}
