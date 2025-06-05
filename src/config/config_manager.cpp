/**
 * @file config_manager.cpp
 * @brief 配置管理器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>

namespace dlogcover {
namespace config {

namespace {
/**
 * @brief 检查JSON对象中是否存在指定的键，并且值类型符合要求
 * @param json JSON对象
 * @param key 键名
 * @param type 期望的值类型
 * @return 如果键存在且类型匹配返回true，否则返回false
 */
bool hasValidField(const nlohmann::json& json, const std::string& key, nlohmann::json::value_t type) {
    auto it = json.find(key);
    return it != json.end() && it->type() == type;
}

/**
 * @brief 从JSON数组中安全地读取字符串列表
 * @param json JSON数组
 * @param defaultValue 默认值
 * @return 字符串列表
 */
std::vector<std::string> safeGetStringArray(const nlohmann::json& json,
                                            const std::vector<std::string>& defaultValue = {}) {
    if (!json.is_array()) {
        return defaultValue;
    }

    std::vector<std::string> result;
    result.reserve(json.size());

    for (const auto& item : json) {
        if (item.is_string()) {
            result.push_back(item.get<std::string>());
        }
    }

    return result.empty() ? defaultValue : result;
}
}  // namespace

ConfigManager::ConfigManager() {
    LOG_DEBUG("配置管理器初始化");
    config_ = getDefaultConfig();
}

ConfigManager::~ConfigManager() {
    LOG_DEBUG("配置管理器销毁");
}

bool ConfigManager::parseScanConfig(const nlohmann::json& jsonConfig) {
    LOG_DEBUG("解析扫描配置");
    try {
        if (!hasValidField(jsonConfig, "scan", nlohmann::json::value_t::object)) {
            return true;  // 使用默认配置
        }

        const auto& scan = jsonConfig["scan"];

        if (hasValidField(scan, "directories", nlohmann::json::value_t::array)) {
            config_.scan.directories = safeGetStringArray(scan["directories"], config_.scan.directories);
        }

        if (hasValidField(scan, "excludes", nlohmann::json::value_t::array)) {
            config_.scan.excludes = safeGetStringArray(scan["excludes"], config_.scan.excludes);
        }

        if (hasValidField(scan, "file_types", nlohmann::json::value_t::array)) {
            config_.scan.fileTypes = safeGetStringArray(scan["file_types"], config_.scan.fileTypes);
        }

        // 添加对include_paths的解析
        if (hasValidField(scan, "include_paths", nlohmann::json::value_t::string)) {
            config_.scan.includePathsStr = scan["include_paths"].get<std::string>();
            LOG_DEBUG_FMT("包含路径设置为: %s", config_.scan.includePathsStr.c_str());
        }

        // 添加对is_qt_project的解析
        if (hasValidField(scan, "is_qt_project", nlohmann::json::value_t::boolean)) {
            config_.scan.isQtProject = scan["is_qt_project"].get<bool>();
            LOG_DEBUG_FMT("Qt项目标志设置为: %s", config_.scan.isQtProject ? "true" : "false");
        }

        // 添加对compiler_args的解析
        if (hasValidField(scan, "compiler_args", nlohmann::json::value_t::array)) {
            config_.scan.compilerArgs = safeGetStringArray(scan["compiler_args"], config_.scan.compilerArgs);
            LOG_DEBUG_FMT("添加了 %zu 个编译器参数", config_.scan.compilerArgs.size());
        }

        // 添加对cmake配置的解析
        if (hasValidField(scan, "cmake", nlohmann::json::value_t::object)) {
            const auto& cmake = scan["cmake"];
            
            if (hasValidField(cmake, "enabled", nlohmann::json::value_t::boolean)) {
                config_.scan.cmake.enabled = cmake["enabled"].get<bool>();
                LOG_DEBUG_FMT("CMake自动检测设置为: %s", config_.scan.cmake.enabled ? "启用" : "禁用");
            }
            
            if (hasValidField(cmake, "cmake_lists_path", nlohmann::json::value_t::string)) {
                config_.scan.cmake.cmakeListsPath = cmake["cmake_lists_path"].get<std::string>();
                LOG_DEBUG_FMT("CMakeLists.txt路径设置为: %s", config_.scan.cmake.cmakeListsPath.c_str());
            }
            
            if (hasValidField(cmake, "target_name", nlohmann::json::value_t::string)) {
                config_.scan.cmake.targetName = cmake["target_name"].get<std::string>();
                LOG_DEBUG_FMT("CMake目标名称设置为: %s", config_.scan.cmake.targetName.c_str());
            }
            
            if (hasValidField(cmake, "use_compile_commands", nlohmann::json::value_t::boolean)) {
                config_.scan.cmake.useCompileCommands = cmake["use_compile_commands"].get<bool>();
                LOG_DEBUG_FMT("使用compile_commands.json设置为: %s", config_.scan.cmake.useCompileCommands ? "启用" : "禁用");
            }
            
            if (hasValidField(cmake, "compile_commands_path", nlohmann::json::value_t::string)) {
                config_.scan.cmake.compileCommandsPath = cmake["compile_commands_path"].get<std::string>();
                LOG_DEBUG_FMT("compile_commands.json路径设置为: %s", config_.scan.cmake.compileCommandsPath.c_str());
            }
            
            if (hasValidField(cmake, "verbose_logging", nlohmann::json::value_t::boolean)) {
                config_.scan.cmake.verboseLogging = cmake["verbose_logging"].get<bool>();
                LOG_DEBUG_FMT("CMake详细日志设置为: %s", config_.scan.cmake.verboseLogging ? "启用" : "禁用");
            }
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("解析扫描配置失败: %s", e.what());
        return false;
    }
}

bool ConfigManager::parseLogFunctionsConfig(const nlohmann::json& jsonConfig) {
    LOG_DEBUG("解析日志函数配置");
    try {
        if (!hasValidField(jsonConfig, "log_functions", nlohmann::json::value_t::object)) {
            return true;  // 使用默认配置
        }

        const auto& logFunctions = jsonConfig["log_functions"];

        // 解析Qt日志函数配置
        if (hasValidField(logFunctions, "qt", nlohmann::json::value_t::object)) {
            const auto& qt = logFunctions["qt"];

            if (hasValidField(qt, "enabled", nlohmann::json::value_t::boolean)) {
                config_.logFunctions.qt.enabled = qt["enabled"].get<bool>();
            }

            if (hasValidField(qt, "functions", nlohmann::json::value_t::array)) {
                config_.logFunctions.qt.functions =
                    safeGetStringArray(qt["functions"], config_.logFunctions.qt.functions);
            }

            if (hasValidField(qt, "category_functions", nlohmann::json::value_t::array)) {
                config_.logFunctions.qt.categoryFunctions =
                    safeGetStringArray(qt["category_functions"], config_.logFunctions.qt.categoryFunctions);
            }
        }

        // 解析自定义日志函数配置
        if (hasValidField(logFunctions, "custom", nlohmann::json::value_t::object)) {
            const auto& custom = logFunctions["custom"];

            if (hasValidField(custom, "enabled", nlohmann::json::value_t::boolean)) {
                config_.logFunctions.custom.enabled = custom["enabled"].get<bool>();
            }

            if (hasValidField(custom, "functions", nlohmann::json::value_t::object)) {
                const auto& functions = custom["functions"];
                for (auto it = functions.begin(); it != functions.end(); ++it) {
                    if (it.value().is_array()) {
                        config_.logFunctions.custom.functions[it.key()] =
                            safeGetStringArray(it.value(), config_.logFunctions.custom.functions[it.key()]);
                    }
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("解析日志函数配置失败: %s", e.what());
        return false;
    }
}

bool ConfigManager::parseAnalysisConfig(const nlohmann::json& jsonConfig) {
    LOG_DEBUG("解析分析配置");
    try {
        if (!hasValidField(jsonConfig, "analysis", nlohmann::json::value_t::object)) {
            return true;  // 使用默认配置
        }

        const auto& analysis = jsonConfig["analysis"];

        if (hasValidField(analysis, "function_coverage", nlohmann::json::value_t::boolean)) {
            config_.analysis.functionCoverage = analysis["function_coverage"].get<bool>();
        }

        if (hasValidField(analysis, "branch_coverage", nlohmann::json::value_t::boolean)) {
            config_.analysis.branchCoverage = analysis["branch_coverage"].get<bool>();
        }

        if (hasValidField(analysis, "exception_coverage", nlohmann::json::value_t::boolean)) {
            config_.analysis.exceptionCoverage = analysis["exception_coverage"].get<bool>();
        }

        if (hasValidField(analysis, "key_path_coverage", nlohmann::json::value_t::boolean)) {
            config_.analysis.keyPathCoverage = analysis["key_path_coverage"].get<bool>();
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("解析分析配置失败: %s", e.what());
        return false;
    }
}

bool ConfigManager::parseReportConfig(const nlohmann::json& jsonConfig) {
    LOG_DEBUG("解析报告配置");
    try {
        if (!hasValidField(jsonConfig, "report", nlohmann::json::value_t::object)) {
            return true;  // 使用默认配置
        }

        const auto& report = jsonConfig["report"];

        if (hasValidField(report, "format", nlohmann::json::value_t::string)) {
            config_.report.format = report["format"].get<std::string>();
        }

        if (hasValidField(report, "timestamp_format", nlohmann::json::value_t::string)) {
            config_.report.timestampFormat = report["timestamp_format"].get<std::string>();
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("解析报告配置失败: %s", e.what());
        return false;
    }
}

bool ConfigManager::loadConfig(const std::string& path) {
    LOG_INFO_FMT("加载配置文件: %s", path.c_str());

    if (!utils::FileUtils::fileExists(path)) {
        LOG_WARNING_FMT("配置文件不存在: %s，将使用默认配置", path.c_str());
        // 使用默认配置
        config_ = getDefaultConfig();
        LOG_INFO("已加载默认配置");
        return true;
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR_FMT("无法打开配置文件: %s，将使用默认配置", path.c_str());
            // 使用默认配置
            config_ = getDefaultConfig();
            LOG_INFO("已加载默认配置");
            return true;
        }

        nlohmann::json jsonConfig;
        file >> jsonConfig;

        // 按顺序解析各个配置部分
        if (!parseScanConfig(jsonConfig) || !parseLogFunctionsConfig(jsonConfig) || !parseAnalysisConfig(jsonConfig) ||
            !parseReportConfig(jsonConfig)) {
            LOG_ERROR_FMT("配置文件解析失败: %s，将使用默认配置", path.c_str());
            // 使用默认配置
            config_ = getDefaultConfig();
            LOG_INFO("已加载默认配置");
            return true;
        }

        LOG_INFO("配置文件加载成功");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("配置文件解析错误: %s，将使用默认配置", e.what());
        // 使用默认配置
        config_ = getDefaultConfig();
        LOG_INFO("已加载默认配置");
        return true;
    }
}

void ConfigManager::mergeWithCommandLineOptions(const cli::Options& options) {
    LOG_DEBUG("与命令行选项合并");

    // 合并扫描目录
    if (!options.directoryPath.empty()) {
        if (config_.scan.directories.empty()) {
            config_.scan.directories.push_back(options.directoryPath);
        } else {
            config_.scan.directories[0] = options.directoryPath;
        }
    }

    // 合并排除模式
    if (!options.excludePatterns.empty()) {
        config_.scan.excludes.insert(config_.scan.excludes.end(), options.excludePatterns.begin(),
                                     options.excludePatterns.end());
    }

    // 合并日志级别过滤
    if (options.logLevel != cli::LogLevel::ALL) {
        updateLogLevelFilters(options.logLevel);
    }

    // 合并报告格式
    config_.report.format = (options.reportFormat == cli::ReportFormat::JSON) ? "json" : "text";
}

void ConfigManager::updateLogLevelFilters(cli::LogLevel level) {
    LOG_DEBUG_FMT("更新日志级别过滤器: %d", static_cast<int>(level));

    const std::vector<std::string> levelOrder = {"debug", "info", "warning", "critical", "fatal"};
    size_t startIndex = 0;

    switch (level) {
        case cli::LogLevel::DEBUG:
            startIndex = 0;
            break;
        case cli::LogLevel::INFO:
            startIndex = 1;
            break;
        case cli::LogLevel::WARNING:
            startIndex = 2;
            break;
        case cli::LogLevel::CRITICAL:
            startIndex = 3;
            break;
        case cli::LogLevel::FATAL:
            startIndex = 4;
            break;
        default:
            return;
    }

    // 移除低于指定级别的日志函数
    for (size_t i = 0; i < startIndex; ++i) {
        const auto& levelName = levelOrder[i];
        auto it = config_.logFunctions.custom.functions.find(levelName);
        if (it != config_.logFunctions.custom.functions.end()) {
            config_.logFunctions.custom.functions.erase(it);
        }
    }
}

const Config& ConfigManager::getConfig() const {
    return config_;
}

Config ConfigManager::getDefaultConfig() {
    LOG_DEBUG("创建默认配置");

    Config config;

    // 默认扫描配置
    config.scan.directories = {"./"};  // 默认为当前目录
    config.scan.excludes = {"build/", "test/", "third_party/", ".git/"};
    config.scan.fileTypes = {".cpp", ".cc", ".cxx", ".h", ".hpp"};
    config.scan.includePathsStr = "./include:./src";  // 默认包含路径
    config.scan.isQtProject = false;                  // 默认不是Qt项目
    config.scan.compilerArgs = {                      // 默认编译参数
                                "-Wno-everything", "-xc++", "-ferror-limit=0", "-fsyntax-only"};
    
    // 默认CMake配置
    config.scan.cmake.enabled = true;                 // 默认启用CMake参数自动检测
    config.scan.cmake.cmakeListsPath = "";            // 空则自动查找
    config.scan.cmake.targetName = "";                // 空则使用全局参数
    config.scan.cmake.useCompileCommands = true;      // 默认使用compile_commands.json
    config.scan.cmake.compileCommandsPath = "";       // 空则自动查找
    config.scan.cmake.verboseLogging = false;         // 默认不启用详细日志

    // 默认Qt日志函数配置
    config.logFunctions.qt.enabled = true;
    config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};
    config.logFunctions.qt.categoryFunctions = {"qCDebug", "qCInfo", "qCWarning", "qCCritical"};

    // 默认自定义日志函数配置
    config.logFunctions.custom.enabled = true;
    config.logFunctions.custom.functions["debug"] = {"logDebug", "LOG_DEBUG", "LOG_DEBUG_FMT"};
    config.logFunctions.custom.functions["info"] = {"logInfo", "LOG_INFO", "LOG_INFO_FMT"};
    config.logFunctions.custom.functions["warning"] = {"logWarning", "LOG_WARNING", "LOG_WARNING_FMT"};
    config.logFunctions.custom.functions["critical"] = {"logCritical"};
    config.logFunctions.custom.functions["fatal"] = {"logFatal", "LOG_ERROR", "LOG_ERROR_FMT", "LOG_FATAL",
                                                     "LOG_FATAL_FMT"};

    // 默认分析配置
    config.analysis.functionCoverage = true;
    config.analysis.branchCoverage = true;
    config.analysis.exceptionCoverage = true;
    config.analysis.keyPathCoverage = true;

    // 默认报告配置
    config.report.format = "text";
    config.report.timestampFormat = "YYYYMMDD_HHMMSS";

    return config;
}

bool ConfigManager::validateConfig() const {
    LOG_DEBUG("验证配置");

    return validateScanConfig() && validateLogFunctionsConfig() && validateReportConfig();
}

bool ConfigManager::validateScanConfig() const {
    // 检查扫描目录是否存在
    for (const auto& dir : config_.scan.directories) {
        if (!utils::FileUtils::directoryExists(dir)) {
            // 尝试创建目录
            LOG_WARNING_FMT("扫描目录不存在，尝试创建: %s", dir.c_str());
            try {
                if (std::filesystem::create_directories(dir)) {
                    LOG_INFO_FMT("成功创建扫描目录: %s", dir.c_str());
                } else {
                    LOG_ERROR_FMT("无法创建扫描目录: %s", dir.c_str());
                    return false;
                }
            } catch (const std::exception& e) {
                LOG_ERROR_FMT("创建扫描目录失败: %s, 错误: %s", dir.c_str(), e.what());
                return false;
            }
        }
    }

    // 检查文件类型
    if (config_.scan.fileTypes.empty()) {
        LOG_ERROR("未指定文件类型");
        return false;
    }

    // 检查文件类型格式
    for (const auto& fileType : config_.scan.fileTypes) {
        if (fileType.empty() || fileType[0] != '.') {
            LOG_ERROR_FMT("文件类型格式错误: %s", fileType.c_str());
            return false;
        }
    }

    return true;
}

bool ConfigManager::validateLogFunctionsConfig() const {
    // 检查日志函数
    if (!config_.logFunctions.qt.enabled && !config_.logFunctions.custom.enabled) {
        LOG_ERROR("未启用任何日志函数");
        return false;
    }

    // 检查Qt日志函数配置
    if (config_.logFunctions.qt.enabled) {
        if (config_.logFunctions.qt.functions.empty() && config_.logFunctions.qt.categoryFunctions.empty()) {
            LOG_ERROR("Qt日志函数配置无效：未指定任何函数");
            return false;
        }
    }

    // 检查自定义日志函数配置
    if (config_.logFunctions.custom.enabled && config_.logFunctions.custom.functions.empty()) {
        LOG_ERROR("自定义日志函数配置无效：未指定任何函数");
        return false;
    }

    return true;
}

bool ConfigManager::validateReportConfig() const {
    // 检查报告格式
    if (config_.report.format != "text" && config_.report.format != "json") {
        LOG_ERROR_FMT("不支持的报告格式: %s", config_.report.format.c_str());
        return false;
    }

    // 检查时间戳格式
    if (config_.report.timestampFormat.empty()) {
        LOG_ERROR("时间戳格式不能为空");
        return false;
    }

    return true;
}

}  // namespace config
}  // namespace dlogcover
