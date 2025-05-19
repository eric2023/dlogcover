/**
 * @file command_line_parser.cpp
 * @brief 命令行解析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 *
 * 该文件实现了命令行参数解析器，用于处理DLogCover工具的命令行参数。
 * 主要功能包括：
 * - 解析命令行参数
 * - 验证参数有效性
 * - 设置默认值
 * - 处理帮助和版本信息请求
 *
 * 使用示例：
 * @code
 * int main(int argc, char** argv) {
 *     CommandLineParser parser;
 *     if (!parser.parse(argc, argv)) {
 *         return 1;
 *     }
 *     const Options& options = parser.getOptions();
 *     // 使用解析后的选项...
 *     return 0;
 * }
 * @endcode
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/cli/config_validator.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/string_utils.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace dlogcover {
namespace cli {

namespace {
/**
 * @brief 版本信息
 * @note 版本号格式：主版本号.次版本号.修订号
 */
const std::string VERSION = "0.1.0";

/**
 * @brief 命令行选项常量
 * @{
 */
const char* const OPTION_HELP_SHORT = "-h";
const char* const OPTION_HELP_LONG = "--help";
const char* const OPTION_VERSION_SHORT = "-v";
const char* const OPTION_VERSION_LONG = "--version";
const char* const OPTION_DIRECTORY_SHORT = "-d";
const char* const OPTION_DIRECTORY_LONG = "--directory";
const char* const OPTION_OUTPUT_SHORT = "-o";
const char* const OPTION_OUTPUT_LONG = "--output";
const char* const OPTION_CONFIG_SHORT = "-c";
const char* const OPTION_CONFIG_LONG = "--config";
const char* const OPTION_EXCLUDE_SHORT = "-e";
const char* const OPTION_EXCLUDE_LONG = "--exclude";
const char* const OPTION_LOG_LEVEL_SHORT = "-l";
const char* const OPTION_LOG_LEVEL_LONG = "--log-level";
const char* const OPTION_FORMAT_SHORT = "-f";
const char* const OPTION_FORMAT_LONG = "--format";
const char* const OPTION_LOG_PATH_SHORT = "-p";
const char* const OPTION_LOG_PATH_LONG = "--log-path";
/** @} */

/**
 * @brief 默认值常量
 * @{
 */
const char* const DEFAULT_DIRECTORY = "./";
const char* const DEFAULT_OUTPUT = "./dlogcover_report.txt";
const char* const DEFAULT_CONFIG = "./dlogcover.json";
/** @} */

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
  -p, --log-path <path>      指定日志文件路径 (默认: dlogcover_<timestamp>.log)

环境变量:
  DLOGCOVER_DIRECTORY       等同于 -d 选项
  DLOGCOVER_OUTPUT          等同于 -o 选项
  DLOGCOVER_CONFIG          等同于 -c 选项
  DLOGCOVER_LOG_LEVEL       等同于 -l 选项
  DLOGCOVER_REPORT_FORMAT   等同于 -f 选项
  DLOGCOVER_EXCLUDE         排除模式列表，使用冒号分隔
  DLOGCOVER_LOG_PATH        等同于 -p 选项

配置文件格式 (JSON):
{
  "directory": "./src",              // 必需，扫描目录
  "output": "./report.txt",          // 可选，输出路径
  "log_level": "debug",             // 可选，日志级别
  "report_format": "text",          // 可选，报告格式
  "exclude": ["build/*", "test/*"]  // 可选，排除模式列表
  "log_path": "./dlogcover.log"     // 可选，日志文件路径
}

优先级:
  命令行参数 > 配置文件 > 环境变量 > 默认值

示例:
  dlogcover -d /path/to/source -o report.txt
  dlogcover -c config.json -e "build/*" -e "test/*"
  DLOGCOVER_DIRECTORY=./src DLOGCOVER_LOG_LEVEL=debug dlogcover
)";

/**
 * @brief 日志级别字符串映射表
 * 用于将命令行参数中的字符串转换为对应的日志级别枚举值
 */
const std::map<std::string, LogLevel> LOG_LEVEL_MAP = {{"debug", LogLevel::DEBUG},     {"info", LogLevel::INFO},
                                                       {"warning", LogLevel::WARNING}, {"critical", LogLevel::CRITICAL},
                                                       {"fatal", LogLevel::FATAL},     {"all", LogLevel::ALL}};

/**
 * @brief 报告格式字符串映射表
 * 用于将命令行参数中的字符串转换为对应的报告格式枚举值
 */
const std::map<std::string, ReportFormat> REPORT_FORMAT_MAP = {{"text", ReportFormat::TEXT},
                                                               {"json", ReportFormat::JSON}};

/**
 * @brief 解析日志级别字符串
 * @param level 日志级别字符串
 * @return 对应的日志级别枚举值
 */
LogLevel parseLogLevelImpl(std::string_view level) {
    std::string levelLower{level};
    std::transform(levelLower.begin(), levelLower.end(), levelLower.begin(), ::tolower);

    if (levelLower == config::log::DEBUG)
        return LogLevel::DEBUG;
    else if (levelLower == config::log::INFO)
        return LogLevel::INFO;
    else if (levelLower == config::log::WARNING)
        return LogLevel::WARNING;
    else if (levelLower == config::log::CRITICAL)
        return LogLevel::CRITICAL;
    else if (levelLower == config::log::FATAL)
        return LogLevel::FATAL;
    else if (levelLower == config::log::ALL)
        return LogLevel::ALL;

    throw std::invalid_argument(std::string(config::error::INVALID_LOG_LEVEL) + std::string(level));
}

/**
 * @brief 解析报告格式字符串
 * @param format 报告格式字符串
 * @return 对应的报告格式枚举值
 */
ReportFormat parseReportFormatImpl(std::string_view format) {
    std::string formatLower{format};
    std::transform(formatLower.begin(), formatLower.end(), formatLower.begin(), ::tolower);

    if (formatLower == config::report::TEXT)
        return ReportFormat::TEXT;
    else if (formatLower == config::report::JSON)
        return ReportFormat::JSON;

    throw std::invalid_argument(std::string(config::error::INVALID_REPORT_FORMAT) + std::string(format));
}
}  // namespace

/**
 * @brief 构造函数
 * @details 初始化命令行解析器，设置所有选项的默认值
 */
CommandLineParser::CommandLineParser() {
    LOG_DEBUG("创建命令行解析器实例");
    // 设置默认值
    options_.directoryPath = DEFAULT_DIRECTORY;
    options_.outputPath = DEFAULT_OUTPUT;
    options_.configPath = DEFAULT_CONFIG;
    options_.logLevel = LogLevel::ALL;
    options_.reportFormat = ReportFormat::TEXT;
    options_.logPath = "";  // 默认为空，将使用自动生成的文件名

    // 初始化帮助和版本请求标志
    isHelpRequest_ = false;
    isVersionRequest_ = false;
}

/**
 * @brief 析构函数
 */
CommandLineParser::~CommandLineParser() {
    LOG_DEBUG("销毁命令行解析器实例");
}

/**
 * @brief 解析命令行参数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return 解析成功返回true，否则返回false
 * @details 该函数会解析命令行参数，并根据参数设置相应的选项。
 *          如果遇到帮助或版本请求，会显示相应信息并返回false。
 *          如果参数无效或缺失，会输出错误信息并返回false。
 *
 * @note 参数解析规则：
 * - 无参数时使用默认值
 * - 短格式选项使用单个连字符（如 -h）
 * - 长格式选项使用双连字符（如 --help）
 * - 需要值的选项在选项后跟一个空格和值
 * - 排除模式选项可以多次使用
 *
 * @warning 该函数会验证文件和目录路径的有效性，如果路径无效会返回false
 */
ErrorResult CommandLineParser::parse(int argc, char** argv) {
    LOG_DEBUG_FMT("开始解析命令行参数，参数数量: %d", argc);

    // 首先从环境变量加载选项
    ConfigValidator validator;
    if (!validator.loadFromEnvironment(options_)) {
        return ErrorResult(validator.getErrorCode(), validator.getError());
    }

    if (argc <= 1) {
        LOG_INFO("无命令行参数，使用默认配置");
        return ErrorResult();  // 使用默认值和环境变量
    }

    std::vector<std::string> args;
    args.reserve(argc - 1);  // 预分配空间
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);  // 使用 emplace_back
    }

    try {
        // 首先处理配置文件参数，因为其他参数可能会覆盖配置文件中的值
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string_view arg = args[i];
            if (arg == config::cli::OPTION_CONFIG_SHORT || arg == config::cli::OPTION_CONFIG_LONG) {
                auto result =
                    handleOption(args, i, arg, [this, &validator](std::string_view configPath) -> ErrorResult {
                        options_.configPath = std::string(configPath);
                        if (auto pathResult = validatePath(configPath, false); pathResult.hasError()) {
                            return pathResult;
                        }
                        if (!validator.loadFromConfig(configPath, options_)) {
                            return ErrorResult(validator.getErrorCode(), validator.getError());
                        }
                        return ErrorResult();
                    });
                if (result.hasError()) {
                    return result;
                }
            }
        }

        // 处理其他参数
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string_view arg = args[i];

            if (arg == config::cli::OPTION_HELP_SHORT || arg == config::cli::OPTION_HELP_LONG) {
                isHelpRequest_ = true;
                return ErrorResult();
            } else if (arg == config::cli::OPTION_VERSION_SHORT || arg == config::cli::OPTION_VERSION_LONG) {
                isVersionRequest_ = true;
                return ErrorResult();
            } else if (arg == config::cli::OPTION_DIRECTORY_SHORT || arg == config::cli::OPTION_DIRECTORY_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view dirPath) -> ErrorResult {
                    options_.directoryPath = std::string(dirPath);
                    return validatePath(dirPath, true);
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_OUTPUT_SHORT || arg == config::cli::OPTION_OUTPUT_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view outputPath) -> ErrorResult {
                    options_.outputPath = std::string(outputPath);
                    std::filesystem::path fsPath(outputPath);
                    auto parentPath = fsPath.parent_path();
                    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
                        return ErrorResult(ConfigError::OutputDirectoryNotFound,
                                           std::string(config::error::OUTPUT_DIR_NOT_FOUND) + parentPath.string());
                    }
                    return ErrorResult();
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_LOG_PATH_SHORT || arg == config::cli::OPTION_LOG_PATH_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view logPath) -> ErrorResult {
                    options_.logPath = std::string(logPath);
                    std::filesystem::path fsPath(logPath);
                    auto parentPath = fsPath.parent_path();
                    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
                        return ErrorResult(ConfigError::InvalidLogPath,
                                           std::string(config::error::INVALID_LOG_PATH) + parentPath.string());
                    }
                    return ErrorResult();
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_EXCLUDE_SHORT || arg == config::cli::OPTION_EXCLUDE_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view pattern) -> ErrorResult {
                    if (pattern.empty()) {
                        return ErrorResult(ConfigError::InvalidExcludePattern,
                                           std::string(config::error::INVALID_EXCLUDE));
                    }
                    options_.excludePatterns.emplace_back(pattern);
                    return ErrorResult();
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_LOG_LEVEL_SHORT || arg == config::cli::OPTION_LOG_LEVEL_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view levelStr) -> ErrorResult {
                    try {
                        options_.logLevel = parseLogLevelImpl(levelStr);
                        return ErrorResult();
                    } catch (const std::invalid_argument& e) {
                        return ErrorResult(ConfigError::InvalidLogLevel,
                                           std::string(config::error::INVALID_LOG_LEVEL) + std::string(levelStr));
                    }
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_FORMAT_SHORT || arg == config::cli::OPTION_FORMAT_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view formatStr) -> ErrorResult {
                    try {
                        options_.reportFormat = parseReportFormatImpl(formatStr);
                        return ErrorResult();
                    } catch (const std::invalid_argument& e) {
                        return ErrorResult(ConfigError::InvalidReportFormat,
                                           std::string(config::error::INVALID_REPORT_FORMAT) + std::string(formatStr));
                    }
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg[0] == '-') {
                return ErrorResult(ConfigError::UnknownOption,
                                   std::string(config::error::UNKNOWN_OPTION) + std::string(arg));
            }
        }

        return options_.validate();
    } catch (const std::exception& e) {
        return ErrorResult(ConfigError::ParseError, std::string(config::error::PARSE_ERROR) + e.what());
    }
}

/**
 * @brief 显示帮助信息
 * @details 在标准输出中显示程序的使用说明、可用选项及示例
 */
void CommandLineParser::showHelp() const {
    std::cout << HELP_TEXT << std::endl;
}

/**
 * @brief 显示版本信息
 * @details 在标准输出中显示程序的版本号
 */
void CommandLineParser::showVersion() const {
    std::cout << "DLogCover v" << VERSION << std::endl;
}

const Options& CommandLineParser::getOptions() const {
    return options_;
}

bool CommandLineParser::isHelpOrVersionRequest() const {
    return isHelpRequest_ || isVersionRequest_;
}

/**
 * @brief 验证路径是否存在
 * @param path 路径
 * @param isDirectory 是否是目录
 * @return 错误结果
 */
ErrorResult CommandLineParser::validatePath(std::string_view path, bool isDirectory) {
    if (path.empty()) {
        return ErrorResult();  // 空路径允许使用默认值
    }

    std::filesystem::path fsPath(path);
    if (!std::filesystem::exists(fsPath)) {
        if (isDirectory) {
            return ErrorResult(ConfigError::DirectoryNotFound,
                               std::string(config::error::DIRECTORY_NOT_FOUND) + std::string(path));
        } else {
            return ErrorResult(ConfigError::FileNotFound,
                               std::string(config::error::FILE_NOT_FOUND) + std::string(path));
        }
    }

    if (isDirectory && !std::filesystem::is_directory(fsPath)) {
        return ErrorResult(ConfigError::DirectoryNotFound,
                           std::string(config::error::DIRECTORY_NOT_FOUND) + std::string(path));
    }

    return ErrorResult();
}

}  // namespace cli
}  // namespace dlogcover
