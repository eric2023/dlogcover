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
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/log_utils.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

using namespace dlogcover;

int main(int argc, char* argv[]) {
    try {
        // 初始化日志系统
        std::string logFileName = "dlogcover.log";
        utils::Logger::init(logFileName, true, utils::LogLevel::INFO);
        LOG_INFO("DLogCover启动");

        // 解析命令行参数
        cli::CommandLineParser commandLineParser;
        if (!commandLineParser.parse(argc, argv)) {
            // 检查是否是帮助或版本请求
            if (commandLineParser.isHelpOrVersionRequest()) {
                return 0;  // 帮助或版本请求，正常退出
            }
            return 1;  // 其他解析错误
        }

        // 获取命令行选项
        const cli::Options& options = commandLineParser.getOptions();

        // 加载配置文件
        config::ConfigManager configManager;
        if (!configManager.loadConfig(options.configPath)) {
            LOG_ERROR("无法加载配置文件: " + options.configPath);
            return 1;
        }

        // 与命令行选项合并
        configManager.mergeWithCommandLineOptions(options);

        // 验证配置
        if (!configManager.validateConfig()) {
            LOG_ERROR("配置验证失败");
            return 1;
        }

        const config::Config& config = configManager.getConfig();

        // 创建源文件管理器
        LOG_INFO("开始收集源文件");
        source_manager::SourceManager sourceManager(config);
        if (!sourceManager.collectSourceFiles()) {
            LOG_ERROR("源文件收集失败");
            return 1;
        }
        LOG_INFO_FMT("共收集到%lu个源文件", sourceManager.getSourceFileCount());

        // 创建AST分析器
        LOG_INFO("开始AST分析");
        core::ast_analyzer::ASTAnalyzer astAnalyzer(config, sourceManager);
        if (!astAnalyzer.analyzeAll()) {
            LOG_ERROR("AST分析失败");
            return 1;
        }
        LOG_INFO("AST分析完成");

        // 创建日志识别器
        LOG_INFO("开始识别日志调用");
        core::log_identifier::LogIdentifier logIdentifier(config, astAnalyzer);
        if (!logIdentifier.identifyLogCalls()) {
            LOG_ERROR("日志调用识别失败");
            return 1;
        }
        LOG_INFO("日志调用识别完成");

        // 创建覆盖率计算器
        LOG_INFO("开始计算覆盖率");
        core::coverage::CoverageCalculator coverageCalculator(config, astAnalyzer, logIdentifier);
        if (!coverageCalculator.calculate()) {
            LOG_ERROR("覆盖率计算失败");
            return 1;
        }
        LOG_INFO("覆盖率计算完成");

        // 生成报告
        LOG_INFO("开始生成报告");
        reporter::Reporter reporter(config, coverageCalculator);
        if (!reporter.generateReport(options.outputPath)) {
            LOG_ERROR("报告生成失败");
            return 1;
        }
        LOG_INFO("报告生成完成: " + options.outputPath);

        LOG_INFO("DLogCover执行完成");
        utils::Logger::shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        LOG_ERROR(std::string("异常: ") + e.what());
        utils::Logger::shutdown();
        return 1;
    } catch (...) {
        std::cerr << "未知错误" << std::endl;
        LOG_ERROR("未知异常");
        utils::Logger::shutdown();
        return 1;
    }
}
