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
#include <dlogcover/cli/config_constants.h>
#include <dlogcover/utils/string_utils.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// 包含日志工具，但仅在logParsedOptions函数中使用
// 在其他地方不应该使用日志函数，因为日志系统可能未初始化
#include <dlogcover/utils/log_utils.h>

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
const char* const OPTION_INCLUDE_SHORT = "-i";
const char* const OPTION_INCLUDE_LONG = "--include";
const char* const OPTION_LOG_LEVEL_SHORT = "-l";
const char* const OPTION_LOG_LEVEL_LONG = "--log-level";
const char* const OPTION_FORMAT_SHORT = "-f";
const char* const OPTION_FORMAT_LONG = "--format";
const char* const OPTION_LOG_PATH_SHORT = "-p";
const char* const OPTION_LOG_PATH_LONG = "--log-path";
const char* const OPTION_INCLUDE_PATH_SHORT = "-I";
const char* const OPTION_INCLUDE_PATH_LONG = "--include-path";
const char* const OPTION_QUIET_SHORT = "-q";
const char* const OPTION_QUIET_LONG = "--quiet";
const char* const OPTION_VERBOSE_LONG = "--verbose";
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

主要选项:
  -h, --help                 显示帮助信息
  -v, --version              显示版本信息
  -d, --directory <path>     指定扫描目录 (默认: ./)
  -o, --output <path>        指定输出报告路径 (默认: ./dlogcover_report_<timestamp>.txt)
  -c, --config <path>        指定配置文件路径 (默认: ./dlogcover.json)
  -e, --exclude <pattern>    排除符合模式的文件或目录 (可多次使用)
  -i, --include <pattern>    仅扫描指定的文件或目录 (可多次使用，优先生效)
  -l, --log-level <level>    指定最低日志级别进行过滤 (debug, info, warning, critical, fatal, all)
  -f, --format <format>      指定报告格式 (text, json)
  -p, --log-path <path>      指定日志文件路径 (默认: dlogcover_<timestamp>.log)
  -I, --include-path <path>  头文件搜索路径 (可多次使用)
  -m, --mode <mode>          分析模式 (cpp_only, go_only, auto_detect, 默认: cpp_only)
  -q, --quiet                静默模式，减少输出信息
      --verbose              详细输出模式，显示更多调试信息

性能选项:
      --max-threads <num>    设置最大线程数 (默认: 0=自动检测)
      --disable-parallel     禁用并行分析，强制使用单线程模式
      --disable-cache        禁用AST缓存，每次都重新解析
      --max-cache-size <num> 设置最大缓存条目数 (默认: 100)
      --disable-io-opt       禁用I/O优化，使用标准文件读取

环境变量:
  DLOGCOVER_DIRECTORY       等同于 -d 选项
  DLOGCOVER_OUTPUT          等同于 -o 选项
  DLOGCOVER_CONFIG          等同于 -c 选项
  DLOGCOVER_LOG_LEVEL       等同于 -l 选项
  DLOGCOVER_REPORT_FORMAT   等同于 -f 选项
  DLOGCOVER_EXCLUDE         排除模式列表，使用冒号分隔
  DLOGCOVER_LOG_PATH        等同于 -p 选项

配置文件格式 (JSON):
支持两种格式：

1. 完整嵌套格式 (推荐):
{
  "project": {
    "name": "my-project",
    "directory": "./src",
    "build_directory": "./build"
  },
  "scan": {
    "directories": ["src", "include"],
    "file_extensions": [".cpp", ".h"],
    "exclude_patterns": ["build/*", "test/*"]
  },
  "output": {
    "report_file": "./report.txt",
    "log_file": "./dlogcover.log",
    "log_level": "debug"
  },
  "analysis": {
    "mode": "cpp_only"
  },
  "log_functions": {
    "qt": {
      "enabled": true,
      "functions": ["qDebug", "qInfo"]
    },
    "custom": {
      "enabled": true,
      "functions": {
        "debug": ["LOG_DEBUG"],
        "info": ["LOG_INFO"]
      }
    }
  }
}

2. 简化格式 (兼容):
{
  "directory": "./src",
  "output": "./report.txt",
  "log_level": "debug",
  "exclude": ["build/*", "test/*"],
  "log_path": "./dlogcover.log"
}

优先级:
  命令行参数 > 配置文件 > 环境变量 > 默认值

示例:
  dlogcover -d /path/to/source -o report.txt
  dlogcover -c config.json -e "build/*" -e "test/*"
  dlogcover -I ./include -q
  dlogcover --verbose -f json -o report.json
  dlogcover -d ./src --mode cpp_only
  dlogcover -d ./src --mode go_only
  dlogcover -d ./src --mode auto_detect
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
    // 初始化帮助和版本请求标志
    isHelpRequest_ = false;
    isVersionRequest_ = false;
}

/**
 * @brief 析构函数
 */
CommandLineParser::~CommandLineParser() {
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
    // 首先从环境变量加载选项
    ConfigValidator validator;
    if (!validator.loadFromEnvironment(options_)) {
        return ErrorResult(validator.getErrorCode(), validator.getError());
    }

    if (argc <= 1) {
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
                options_.showHelp = true;  // 同步设置 Options 字段
                showHelp();  // 立即显示帮助
                return ErrorResult(ConfigError::None, "帮助请求");
            } else if (arg == config::cli::OPTION_VERSION_SHORT || arg == config::cli::OPTION_VERSION_LONG) {
                isVersionRequest_ = true;
                options_.showVersion = true;  // 同步设置 Options 字段
                showVersion();  // 立即显示版本
                return ErrorResult(ConfigError::None, "版本请求");
            } else if (arg == config::cli::OPTION_CONFIG_SHORT || arg == config::cli::OPTION_CONFIG_LONG) {
                // 配置文件参数已在第一个循环中处理，这里跳过
                if (i + 1 < args.size()) {
                    ++i; // 跳过参数值
                }
            } else if (arg == config::cli::OPTION_DIRECTORY_SHORT || arg == config::cli::OPTION_DIRECTORY_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view dirPath) -> ErrorResult {
                    options_.directory = std::string(dirPath);
                    return validatePath(dirPath, true);
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_OUTPUT_SHORT || arg == config::cli::OPTION_OUTPUT_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view outputPath) -> ErrorResult {
                    options_.output_file = std::string(outputPath);
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
                    options_.log_file = std::string(logPath);
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
            }

            // 新增include参数分支
            else if (arg == config::cli::OPTION_INCLUDE_SHORT || arg == config::cli::OPTION_INCLUDE_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view pattern) -> ErrorResult {
                    if (pattern.empty()) {
                        return ErrorResult(ConfigError::InvalidExcludePattern,
                                           "include模式不能为空");
                    }
                    options_.includePatterns.emplace_back(pattern);
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
            } else if (arg == config::cli::OPTION_INCLUDE_PATH_SHORT || arg == config::cli::OPTION_INCLUDE_PATH_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view path) -> ErrorResult {
                    options_.includePaths.emplace_back(path);
                    return ErrorResult();
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_QUIET_SHORT || arg == config::cli::OPTION_QUIET_LONG) {
                options_.quiet = true;
            } else if (arg == config::cli::OPTION_VERBOSE_LONG) {
                options_.verbose = true;
            } else if (arg == config::cli::OPTION_MAX_THREADS_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view threadsStr) -> ErrorResult {
                    try {
                        size_t threads = std::stoull(std::string(threadsStr));
                        options_.maxThreads = threads;
                        return ErrorResult();
                    } catch (const std::exception& e) {
                        return ErrorResult(ConfigError::InvalidArgument,
                                           "无效的线程数: " + std::string(threadsStr));
                    }
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_DISABLE_PARALLEL_LONG) {
                options_.disableParallel = true;
            } else if (arg == config::cli::OPTION_DISABLE_CACHE_LONG) {
                options_.disableCache = true;
            } else if (arg == config::cli::OPTION_MAX_CACHE_SIZE_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view sizeStr) -> ErrorResult {
                    try {
                        size_t size = std::stoull(std::string(sizeStr));
                        options_.maxCacheSize = size;
                        return ErrorResult();
                    } catch (const std::exception& e) {
                        return ErrorResult(ConfigError::InvalidArgument,
                                           "无效的缓存大小: " + std::string(sizeStr));
                    }
                });
                if (result.hasError()) {
                    return result;
                }
            } else if (arg == config::cli::OPTION_DISABLE_IO_OPT_LONG) {
                options_.disableIOOptimization = true;
            } else if (arg == config::cli::OPTION_MODE_SHORT || arg == config::cli::OPTION_MODE_LONG) {
                auto result = handleOption(args, i, arg, [this](std::string_view modeStr) -> ErrorResult {
                    std::string mode = std::string(modeStr);
                    if (mode != "cpp_only" && mode != "go_only" && mode != "auto_detect") {
                        return ErrorResult(ConfigError::InvalidArgument,
                                           "无效的分析模式: " + mode + " (支持: cpp_only, go_only, auto_detect)");
                    }
                    options_.mode = mode;
                    return ErrorResult();
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

bool CommandLineParser::isHelpRequest() const {
    return isHelpRequest_;
}

bool CommandLineParser::isVersionRequest() const {
    return isVersionRequest_;
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

/**
 * @brief 将解析后的参数输出到日志文件
 * @details 该函数需要在日志系统初始化后调用，用于记录命令行参数解析结果
 */
void CommandLineParser::logParsedOptions() const {
    // 检查日志系统是否已初始化
    if (!utils::Logger::isInitialized()) {
        return;  // 日志系统未初始化，直接返回
    }

    LOG_INFO("=== 命令行参数解析结果 ===");
    LOG_INFO_FMT("扫描目录: %s", options_.directory.c_str());
    LOG_INFO_FMT("输出路径: %s", options_.output_file.c_str());
    LOG_INFO_FMT("配置文件: %s", options_.configPath.c_str());

    // 记录日志级别
    std::string logLevelStr;
    switch (options_.logLevel) {
        case LogLevel::DEBUG: logLevelStr = "DEBUG"; break;
        case LogLevel::INFO: logLevelStr = "INFO"; break;
        case LogLevel::WARNING: logLevelStr = "WARNING"; break;
        case LogLevel::CRITICAL: logLevelStr = "CRITICAL"; break;
        case LogLevel::FATAL: logLevelStr = "FATAL"; break;
        case LogLevel::ALL: logLevelStr = "ALL"; break;
        default: logLevelStr = "UNKNOWN"; break;
    }
    LOG_INFO_FMT("日志级别: %s", logLevelStr.c_str());

    // 记录报告格式
    std::string formatStr = (options_.reportFormat == ReportFormat::TEXT) ? "TEXT" : "JSON";
    LOG_INFO_FMT("报告格式: %s", formatStr.c_str());

    // 记录日志文件路径
    if (!options_.log_file.empty()) {
        LOG_INFO_FMT("日志文件: %s", options_.log_file.c_str());
    }

    // 记录排除模式
    if (!options_.excludePatterns.empty()) {
        LOG_INFO("排除模式:");
        for (const auto& pattern : options_.excludePatterns) {
            LOG_INFO_FMT("  - %s", pattern.c_str());
        }
    }

    // 记录包含路径
    if (!options_.includePaths.empty()) {
        LOG_INFO("包含路径:");
        for (const auto& path : options_.includePaths) {
            LOG_INFO_FMT("  - %s", path.c_str());
        }
    }

    // 记录其他选项
    LOG_INFO_FMT("静默模式: %s", options_.quiet ? "是" : "否");
    LOG_INFO_FMT("详细模式: %s", options_.verbose ? "是" : "否");

    LOG_INFO("=== 命令行参数记录完成 ===");
}

}  // namespace cli
}  // namespace dlogcover
