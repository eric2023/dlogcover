/**
 * @file config_manager.cpp
 * @brief 配置管理器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>

namespace dlogcover {
namespace config {

ConfigManager::ConfigManager() {
    LOG_DEBUG("配置管理器初始化");
    config_ = getDefaultConfig();
}

ConfigManager::~ConfigManager() {
    LOG_DEBUG("配置管理器销毁");
}

bool ConfigManager::loadConfig(const std::string& path) {
    LOG_INFO_FMT("加载配置文件: %s", path.c_str());

    if (!utils::FileUtils::fileExists(path)) {
        LOG_ERROR_FMT("配置文件不存在: %s", path.c_str());
        return false;
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR_FMT("无法打开配置文件: %s", path.c_str());
            return false;
        }

        nlohmann::json jsonConfig;
        file >> jsonConfig;

        // 解析扫描配置
        if (jsonConfig.find("scan") != jsonConfig.end()) {
            auto& scan = jsonConfig["scan"];

            if (scan.find("directories") != scan.end() && scan["directories"].is_array()) {
                config_.scan.directories.clear();
                for (const auto& dir : scan["directories"]) {
                    config_.scan.directories.push_back(dir.get<std::string>());
                }
            }

            if (scan.find("excludes") != scan.end() && scan["excludes"].is_array()) {
                config_.scan.excludes.clear();
                for (const auto& exclude : scan["excludes"]) {
                    config_.scan.excludes.push_back(exclude.get<std::string>());
                }
            }

            if (scan.find("file_types") != scan.end() && scan["file_types"].is_array()) {
                config_.scan.fileTypes.clear();
                for (const auto& fileType : scan["file_types"]) {
                    config_.scan.fileTypes.push_back(fileType.get<std::string>());
                }
            }
        }

        // 解析日志函数配置
        if (jsonConfig.find("log_functions") != jsonConfig.end()) {
            auto& logFunctions = jsonConfig["log_functions"];

            // Qt日志函数
            if (logFunctions.find("qt") != logFunctions.end()) {
                auto& qt = logFunctions["qt"];

                if (qt.find("enabled") != qt.end() && qt["enabled"].is_boolean()) {
                    config_.logFunctions.qt.enabled = qt["enabled"].get<bool>();
                }

                if (qt.find("functions") != qt.end() && qt["functions"].is_array()) {
                    config_.logFunctions.qt.functions.clear();
                    for (const auto& func : qt["functions"]) {
                        config_.logFunctions.qt.functions.push_back(func.get<std::string>());
                    }
                }

                if (qt.find("category_functions") != qt.end() && qt["category_functions"].is_array()) {
                    config_.logFunctions.qt.categoryFunctions.clear();
                    for (const auto& func : qt["category_functions"]) {
                        config_.logFunctions.qt.categoryFunctions.push_back(func.get<std::string>());
                    }
                }
            }

            // 自定义日志函数
            if (logFunctions.find("custom") != logFunctions.end()) {
                auto& custom = logFunctions["custom"];

                if (custom.find("enabled") != custom.end() && custom["enabled"].is_boolean()) {
                    config_.logFunctions.custom.enabled = custom["enabled"].get<bool>();
                }

                if (custom.find("functions") != custom.end() && custom["functions"].is_object()) {
                    config_.logFunctions.custom.functions.clear();
                    for (auto it = custom["functions"].begin(); it != custom["functions"].end(); ++it) {
                        std::string level = it.key();
                        if (it.value().is_array()) {
                            std::vector<std::string> funcs;
                            for (const auto& func : it.value()) {
                                funcs.push_back(func.get<std::string>());
                            }
                            config_.logFunctions.custom.functions[level] = funcs;
                        }
                    }
                }
            }
        }

        // 解析分析配置
        if (jsonConfig.find("analysis") != jsonConfig.end()) {
            auto& analysis = jsonConfig["analysis"];

            if (analysis.find("function_coverage") != analysis.end() && analysis["function_coverage"].is_boolean()) {
                config_.analysis.functionCoverage = analysis["function_coverage"].get<bool>();
            }

            if (analysis.find("branch_coverage") != analysis.end() && analysis["branch_coverage"].is_boolean()) {
                config_.analysis.branchCoverage = analysis["branch_coverage"].get<bool>();
            }

            if (analysis.find("exception_coverage") != analysis.end() && analysis["exception_coverage"].is_boolean()) {
                config_.analysis.exceptionCoverage = analysis["exception_coverage"].get<bool>();
            }

            if (analysis.find("key_path_coverage") != analysis.end() && analysis["key_path_coverage"].is_boolean()) {
                config_.analysis.keyPathCoverage = analysis["key_path_coverage"].get<bool>();
            }
        }

        // 解析报告配置
        if (jsonConfig.find("report") != jsonConfig.end()) {
            auto& report = jsonConfig["report"];

            if (report.find("format") != report.end() && report["format"].is_string()) {
                config_.report.format = report["format"].get<std::string>();
            }

            if (report.find("timestamp_format") != report.end() && report["timestamp_format"].is_string()) {
                config_.report.timestampFormat = report["timestamp_format"].get<std::string>();
            }
        }

        LOG_INFO("配置文件加载成功");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("配置文件解析错误: %s", e.what());
        return false;
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
        for (const auto& pattern : options.excludePatterns) {
            config_.scan.excludes.push_back(pattern);
        }
    }

    // 合并日志级别过滤
    if (options.logLevel != cli::LogLevel::ALL) {
        // 根据命令行选项调整日志函数配置
        // 例如，如果选择的日志级别是WARNING，则禁用DEBUG和INFO级别的日志函数
        switch (options.logLevel) {
            case cli::LogLevel::DEBUG:
                // 所有级别都保留
                break;
            case cli::LogLevel::INFO:
                // 禁用DEBUG级别
                if (config_.logFunctions.custom.functions.find("debug") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("debug");
                }
                break;
            case cli::LogLevel::WARNING:
                // 禁用DEBUG和INFO级别
                if (config_.logFunctions.custom.functions.find("debug") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("debug");
                }
                if (config_.logFunctions.custom.functions.find("info") != config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("info");
                }
                break;
            case cli::LogLevel::CRITICAL:
                // 只保留CRITICAL和FATAL级别
                if (config_.logFunctions.custom.functions.find("debug") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("debug");
                }
                if (config_.logFunctions.custom.functions.find("info") != config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("info");
                }
                if (config_.logFunctions.custom.functions.find("warning") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("warning");
                }
                break;
            case cli::LogLevel::FATAL:
                // 只保留FATAL级别
                if (config_.logFunctions.custom.functions.find("debug") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("debug");
                }
                if (config_.logFunctions.custom.functions.find("info") != config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("info");
                }
                if (config_.logFunctions.custom.functions.find("warning") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("warning");
                }
                if (config_.logFunctions.custom.functions.find("critical") !=
                    config_.logFunctions.custom.functions.end()) {
                    config_.logFunctions.custom.functions.erase("critical");
                }
                break;
            default:
                break;
        }
    }

    // 合并报告格式
    if (options.reportFormat == cli::ReportFormat::JSON) {
        config_.report.format = "json";
    } else {
        config_.report.format = "text";
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

    // 默认Qt日志函数配置
    config.logFunctions.qt.enabled = true;
    config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};
    config.logFunctions.qt.categoryFunctions = {"qCDebug", "qCInfo", "qCWarning", "qCCritical"};

    // 默认自定义日志函数配置
    config.logFunctions.custom.enabled = true;
    config.logFunctions.custom.functions["debug"] = {"logDebug", "LOG_DEBUG", "LOG_DEBUG_FMT"};
    config.logFunctions.custom.functions["info"] = {"logInfo", "LOG_INFO", "LOG_INFO_FMT"};
    config.logFunctions.custom.functions["warning"] = {"logWarning", "LOG_WARNING", "LOG_WARNING_FMT"};
    config.logFunctions.custom.functions["critical"] = {"logCritical", "LOG_ERROR", "LOG_ERROR_FMT"};
    config.logFunctions.custom.functions["fatal"] = {"logFatal", "LOG_FATAL", "LOG_FATAL_FMT"};

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

    // 检查扫描目录是否存在
    for (const auto& dir : config_.scan.directories) {
        if (!utils::FileUtils::directoryExists(dir)) {
            LOG_ERROR_FMT("扫描目录不存在: %s", dir.c_str());
            return false;
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

    // 检查日志函数
    if (!config_.logFunctions.qt.enabled && !config_.logFunctions.custom.enabled) {
        LOG_ERROR("未启用任何日志函数");
        return false;
    }

    // 检查报告格式
    if (config_.report.format != "text" && config_.report.format != "json") {
        LOG_ERROR_FMT("不支持的报告格式: %s", config_.report.format.c_str());
        return false;
    }

    return true;
}

}  // namespace config
}  // namespace dlogcover
