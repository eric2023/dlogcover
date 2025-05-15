/**
 * @file command_line_parser.cpp
 * @brief 命令行解析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/string_utils.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace dlogcover {
namespace cli {

namespace {
// 版本信息
const std::string VERSION = "0.1.0";
// 帮助信息模板
const std::string HELP_TEXT = R"(
DLogCover - C++代码日志覆盖分析工具 v)" +
                              VERSION + R"(
用法: dlogcover [选项]

选项:
  -h, --help                 显示帮助信息
  -v, --version              显示版本信息
  -d, --directory <path>     指定扫描目录 (默认: ./)
  -o, --output <path>        指定输出报告路径 (默认: ./dlogcover_report_<timestamp>.txt)
  -c, --config <path>        指定配置文件路径 (默认: ./dlogcover.json)
  -e, --exclude <pattern>    排除符合模式的文件或目录 (可多次使用)
  -l, --log-level <level>    指定最低日志级别进行过滤 (debug, info, warning, critical, fatal, all)
  -f, --format <format>      指定报告格式 (text, json)

示例:
  dlogcover -d /path/to/source -o report.txt
  dlogcover -c config.json -e "build/*" -e "test/*"
)";

// 解析日志级别
LogLevel parseLogLevel(const std::string& level) {
    std::string levelLower = level;
    std::transform(levelLower.begin(), levelLower.end(), levelLower.begin(), ::tolower);

    if (levelLower == "debug")
        return LogLevel::DEBUG;
    if (levelLower == "info")
        return LogLevel::INFO;
    if (levelLower == "warning")
        return LogLevel::WARNING;
    if (levelLower == "critical")
        return LogLevel::CRITICAL;
    if (levelLower == "fatal")
        return LogLevel::FATAL;
    if (levelLower == "all")
        return LogLevel::ALL;

    return LogLevel::ALL;  // 默认
}

// 解析报告格式
ReportFormat parseReportFormat(const std::string& format) {
    std::string formatLower = format;
    std::transform(formatLower.begin(), formatLower.end(), formatLower.begin(), ::tolower);

    if (formatLower == "json")
        return ReportFormat::JSON;

    return ReportFormat::TEXT;  // 默认
}
}  // namespace

CommandLineParser::CommandLineParser() {
    // 设置默认值
    options_.directoryPath = "./";
    options_.outputPath = "./dlogcover_report.txt";
    options_.configPath = "./dlogcover.json";
    options_.logLevel = LogLevel::ALL;
    options_.reportFormat = ReportFormat::TEXT;
}

CommandLineParser::~CommandLineParser() {}

bool CommandLineParser::parse(int argc, char** argv) {
    if (argc <= 1) {
        return true;  // 使用默认值
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if (arg == "-h" || arg == "--help") {
            showHelp();
            return false;
        } else if (arg == "-v" || arg == "--version") {
            showVersion();
            return false;
        } else if (arg == "-d" || arg == "--directory") {
            if (i + 1 < args.size()) {
                options_.directoryPath = args[++i];
            } else {
                std::cerr << "错误: 缺少目录路径参数" << std::endl;
                return false;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size()) {
                options_.outputPath = args[++i];
            } else {
                std::cerr << "错误: 缺少输出路径参数" << std::endl;
                return false;
            }
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < args.size()) {
                options_.configPath = args[++i];
            } else {
                std::cerr << "错误: 缺少配置文件路径参数" << std::endl;
                return false;
            }
        } else if (arg == "-e" || arg == "--exclude") {
            if (i + 1 < args.size()) {
                options_.excludePatterns.push_back(args[++i]);
            } else {
                std::cerr << "错误: 缺少排除模式参数" << std::endl;
                return false;
            }
        } else if (arg == "-l" || arg == "--log-level") {
            if (i + 1 < args.size()) {
                options_.logLevel = parseLogLevel(args[++i]);
            } else {
                std::cerr << "错误: 缺少日志级别参数" << std::endl;
                return false;
            }
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < args.size()) {
                options_.reportFormat = parseReportFormat(args[++i]);
            } else {
                std::cerr << "错误: 缺少报告格式参数" << std::endl;
                return false;
            }
        } else {
            std::cerr << "错误: 未知选项: " << arg << std::endl;
            showHelp();
            return false;
        }
    }

    return options_.isValid();
}

void CommandLineParser::showHelp() const {
    std::cout << HELP_TEXT << std::endl;
}

void CommandLineParser::showVersion() const {
    std::cout << "DLogCover 版本 " << VERSION << std::endl;
}

const Options& CommandLineParser::getOptions() const {
    return options_;
}

}  // namespace cli
}  // namespace dlogcover
