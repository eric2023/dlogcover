/**
 * @file config_validator.cpp
 * @brief 配置验证器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/config_validator.h>
#include <dlogcover/utils/log_utils.h>

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace dlogcover {
namespace cli {

namespace {

using json = nlohmann::json;

// 将常量字符串转换为std::string，避免命名空间冲突
const std::string KEY_VERSION_STR{config::json::KEY_VERSION};
const std::string KEY_DIRECTORY_STR{config::json::KEY_DIRECTORY};
const std::string KEY_OUTPUT_STR{config::json::KEY_OUTPUT};
const std::string KEY_LOG_LEVEL_STR{config::json::KEY_LOG_LEVEL};
const std::string KEY_REPORT_FORMAT_STR{config::json::KEY_REPORT_FORMAT};
const std::string KEY_EXCLUDE_STR{config::json::KEY_EXCLUDE};
const std::string KEY_LOG_PATH_STR{config::json::KEY_LOG_PATH};

/**
 * @brief 解析日志级别字符串
 * @param level 日志级别字符串
 * @param result 解析结果
 * @return 解析成功返回true，否则返回false
 */
bool parseLogLevelString(std::string_view level, LogLevel& result) {
    std::string levelLower{level};
    std::transform(levelLower.begin(), levelLower.end(), levelLower.begin(), ::tolower);

    if (levelLower == config::log::DEBUG)
        result = LogLevel::DEBUG;
    else if (levelLower == config::log::INFO)
        result = LogLevel::INFO;
    else if (levelLower == config::log::WARNING)
        result = LogLevel::WARNING;
    else if (levelLower == config::log::CRITICAL)
        result = LogLevel::CRITICAL;
    else if (levelLower == config::log::FATAL)
        result = LogLevel::FATAL;
    else if (levelLower == config::log::ALL)
        result = LogLevel::ALL;
    else
        return false;

    return true;
}

/**
 * @brief 解析报告格式字符串
 * @param format 报告格式字符串
 * @param result 解析结果
 * @return 解析成功返回true，否则返回false
 */
bool parseReportFormatString(std::string_view format, ReportFormat& result) {
    std::string formatLower{format};
    std::transform(formatLower.begin(), formatLower.end(), formatLower.begin(), ::tolower);

    if (formatLower == config::report::TEXT)
        result = ReportFormat::TEXT;
    else if (formatLower == config::report::JSON)
        result = ReportFormat::JSON;
    else
        return false;

    return true;
}

}  // namespace

void ConfigValidator::setError(ConfigError code, const std::string& message) {
    errorCode_ = code;
    error_ = message;
    LOG_ERROR_FMT("配置验证错误[%d]: %s", static_cast<int>(code), message.c_str());
}

bool ConfigValidator::validateVersion(std::string_view version) {
    if (version != config::CURRENT_CONFIG_VERSION) {
        setError(ConfigError::InvalidVersion, std::string(config::error::INVALID_VERSION) + std::string(version));
        return false;
    }
    return true;
}

bool ConfigValidator::validateConfig(std::string_view configPath) {
    LOG_DEBUG_FMT("验证配置文件: %s", configPath.data());

    try {
        std::ifstream file{std::string(configPath)};
        if (!file.is_open()) {
            setError(ConfigError::FileNotFound, std::string(config::error::FILE_OPEN) + std::string(configPath));
            return false;
        }

        json config;
        file >> config;

        // 验证版本
        if (config.find(KEY_VERSION_STR) != config.end()) {
            if (!config[KEY_VERSION_STR].is_string()) {
                setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_VERSION_STR);
                return false;
            }
            configVersion_ = config[KEY_VERSION_STR].get<std::string>();
            if (!validateVersion(configVersion_)) {
                return false;
            }
        }

        // 验证必需字段 - 支持嵌套格式
        std::string directory;
        if (config.find("project") != config.end() && config["project"].find("directory") != config["project"].end()) {
            // 使用嵌套格式: project.directory
            if (!config["project"]["directory"].is_string()) {
                setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + "project.directory");
                return false;
            }
            directory = config["project"]["directory"].get<std::string>();
        } else if (config.find(KEY_DIRECTORY_STR) != config.end()) {
            // 使用简化格式: directory
            if (!config[KEY_DIRECTORY_STR].is_string()) {
                setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_DIRECTORY_STR);
                return false;
            }
            directory = config[KEY_DIRECTORY_STR].get<std::string>();
        } else {
            setError(ConfigError::MissingField, "Missing required field: directory or project.directory");
            return false;
        }

        // 验证输出配置 - 支持嵌套格式
        if (config.find("output") != config.end()) {
            // 检查是嵌套格式还是简化格式
            if (config["output"].is_object()) {
                // 嵌套格式验证
            if (config["output"].find("report_file") != config["output"].end() && 
                !config["output"]["report_file"].is_string()) {
                setError(ConfigError::InvalidType, "output.report_file must be a string");
                return false;
            }
            if (config["output"].find("log_file") != config["output"].end() && 
                !config["output"]["log_file"].is_string()) {
                setError(ConfigError::InvalidType, "output.log_file must be a string");
                return false;
            }
            if (config["output"].find("log_level") != config["output"].end()) {
                if (!config["output"]["log_level"].is_string()) {
                    setError(ConfigError::InvalidType, "output.log_level must be a string");
                    return false;
                }
                LogLevel level;
                if (!parseLogLevelString(config["output"]["log_level"].get<std::string>(), level)) {
                    setError(ConfigError::InvalidLogLevel,
                             std::string(config::error::INVALID_LOG_LEVEL) + config["output"]["log_level"].get<std::string>());
                                         return false;
                 }
             }
            } else if (config["output"].is_string()) {
                // 简化格式：output 是字符串，直接作为输出文件路径
                // 这种情况下不需要额外验证
            } else {
                setError(ConfigError::InvalidType, "output must be either an object or a string");
                return false;
            }
        } else {
            // 简化格式验证
            if (config.find(KEY_OUTPUT_STR) != config.end() && !config[KEY_OUTPUT_STR].is_string()) {
                setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_OUTPUT_STR);
                return false;
            }

            if (config.find(KEY_LOG_PATH_STR) != config.end() && !config[KEY_LOG_PATH_STR].is_string()) {
                setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_LOG_PATH_STR);
                return false;
            }

            if (config.find(KEY_LOG_LEVEL_STR) != config.end()) {
                if (!config[KEY_LOG_LEVEL_STR].is_string()) {
                    setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_LOG_LEVEL_STR);
                    return false;
                }
                LogLevel level;
                if (!parseLogLevelString(config[KEY_LOG_LEVEL_STR].get<std::string>(), level)) {
                    setError(ConfigError::InvalidLogLevel,
                             std::string(config::error::INVALID_LOG_LEVEL) + config[KEY_LOG_LEVEL_STR].get<std::string>());
                    return false;
                }
            }
        }

        if (config.find(KEY_REPORT_FORMAT_STR) != config.end()) {
            if (!config[KEY_REPORT_FORMAT_STR].is_string()) {
                setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_REPORT_FORMAT_STR);
                return false;
            }
            ReportFormat format;
            if (!parseReportFormatString(config[KEY_REPORT_FORMAT_STR].get<std::string>(), format)) {
                setError(ConfigError::InvalidReportFormat, std::string(config::error::INVALID_REPORT_FORMAT) +
                                                               config[KEY_REPORT_FORMAT_STR].get<std::string>());
                return false;
            }
        }

        // 验证扫描配置 - 支持嵌套格式
        if (config.find("scan") != config.end()) {
            // 嵌套格式验证
            if (!config["scan"].is_object()) {
                setError(ConfigError::InvalidType, "scan must be an object");
                return false;
            }
            if (config["scan"].find("exclude_patterns") != config["scan"].end()) {
                if (!config["scan"]["exclude_patterns"].is_array()) {
                    setError(ConfigError::InvalidType, "scan.exclude_patterns must be an array");
                    return false;
                }
                for (const auto& item : config["scan"]["exclude_patterns"]) {
                    if (!item.is_string()) {
                        setError(ConfigError::InvalidExcludePattern, "scan.exclude_patterns items must be strings");
                        return false;
                    }
                }
            }
        } else {
            // 简化格式验证
            if (config.find(KEY_EXCLUDE_STR) != config.end()) {
                if (!config[KEY_EXCLUDE_STR].is_array()) {
                    setError(ConfigError::InvalidType, std::string(config::error::INVALID_TYPE) + KEY_EXCLUDE_STR);
                    return false;
                }
                for (const auto& item : config[KEY_EXCLUDE_STR]) {
                    if (!item.is_string()) {
                        setError(ConfigError::InvalidExcludePattern, std::string(config::error::INVALID_EXCLUDE));
                        return false;
                    }
                }
            }
        }

        LOG_INFO("配置文件验证通过");
        return true;
    } catch (const json::exception& e) {
        setError(ConfigError::ParseError, e.what());
        return false;
    }
}

bool ConfigValidator::loadFromConfig(std::string_view configPath, Options& options) {
    LOG_DEBUG_FMT("从配置文件加载选项: %s", configPath.data());

    if (!validateConfig(configPath)) {
        return false;
    }

    try {
        std::ifstream file{std::string(configPath)};
        json config;
        file >> config;

        // 加载选项 - 支持嵌套格式
        if (config.find("project") != config.end() && config["project"].find("directory") != config["project"].end()) {
            // 使用嵌套格式: project.directory
            options.directory = config["project"]["directory"].get<std::string>();
        } else if (config.find(KEY_DIRECTORY_STR) != config.end()) {
            // 使用简化格式: directory
            options.directory = config[KEY_DIRECTORY_STR].get<std::string>();
        }

        // 支持嵌套和简化格式的输出配置
        if (config.find("output") != config.end()) {
            if (config["output"].is_object() && config["output"].find("report_file") != config["output"].end()) {
                // 使用嵌套格式: output.report_file
                options.output_file = config["output"]["report_file"].get<std::string>();
            } else if (config["output"].is_string()) {
                // 使用简化格式: output 直接是字符串
                options.output_file = config["output"].get<std::string>();
            }
        } else if (config.find(KEY_OUTPUT_STR) != config.end()) {
            // 使用简化格式: output (兼容旧的键名)
            options.output_file = config[KEY_OUTPUT_STR].get<std::string>();
        }

        // 支持嵌套和简化格式的日志路径配置
        if (config.find("output") != config.end() && config["output"].find("log_file") != config["output"].end()) {
            // 使用嵌套格式: output.log_file
            options.log_file = config["output"]["log_file"].get<std::string>();
        } else if (config.find(KEY_LOG_PATH_STR) != config.end()) {
            // 使用简化格式: log_path
            options.log_file = config[KEY_LOG_PATH_STR].get<std::string>();
        }

        // 支持嵌套和简化格式的日志级别配置
        if (config.find("output") != config.end() && config["output"].find("log_level") != config["output"].end()) {
            // 使用嵌套格式: output.log_level
            LogLevel level;
            if (parseLogLevelString(config["output"]["log_level"].get<std::string>(), level)) {
                options.logLevel = level;
            }
        } else if (config.find(KEY_LOG_LEVEL_STR) != config.end()) {
            // 使用简化格式: log_level
            LogLevel level;
            if (parseLogLevelString(config[KEY_LOG_LEVEL_STR].get<std::string>(), level)) {
                options.logLevel = level;
            }
        }

        if (config.find(KEY_REPORT_FORMAT_STR) != config.end()) {
            ReportFormat format;
            if (parseReportFormatString(config[KEY_REPORT_FORMAT_STR].get<std::string>(), format)) {
                options.reportFormat = format;
            }
        }

        // 支持嵌套和简化格式的排除模式配置
        if (config.find("scan") != config.end() && config["scan"].find("exclude_patterns") != config["scan"].end()) {
            // 使用嵌套格式: scan.exclude_patterns
            options.excludePatterns.clear();
            for (const auto& item : config["scan"]["exclude_patterns"]) {
                options.excludePatterns.push_back(item.get<std::string>());
            }
        } else if (config.find(KEY_EXCLUDE_STR) != config.end()) {
            // 使用简化格式: exclude
            options.excludePatterns.clear();
            for (const auto& item : config[KEY_EXCLUDE_STR]) {
                options.excludePatterns.push_back(item.get<std::string>());
            }
        }

        LOG_INFO("从配置文件加载选项成功");
        return true;
    } catch (const json::exception& e) {
        setError(ConfigError::ParseError, e.what());
        return false;
    }
}

bool ConfigValidator::loadFromEnvironment(Options& options) {
    // 只有在日志系统已初始化时才记录日志
    if (utils::Logger::isInitialized()) {
        LOG_DEBUG("从环境变量加载选项");
    }

    try {
        // 获取环境变量
        if (const char* dir = std::getenv(config::env::DIRECTORY)) {
            options.directory = dir;
            if (utils::Logger::isInitialized()) {
                LOG_DEBUG_FMT("从环境变量加载目录路径: %s", dir);
            }
        }

        if (const char* output = std::getenv(config::env::OUTPUT)) {
            options.output_file = output;
            if (utils::Logger::isInitialized()) {
                LOG_DEBUG_FMT("从环境变量加载输出路径: %s", output);
            }
        }

        if (const char* config_path = std::getenv(config::env::CONFIG)) {
            options.configPath = config_path;
            if (utils::Logger::isInitialized()) {
                LOG_DEBUG_FMT("从环境变量加载配置文件路径: %s", config_path);
            }
        }

        if (const char* log_path = std::getenv(config::env::LOG_PATH)) {
            options.log_file = log_path;
            if (utils::Logger::isInitialized()) {
                LOG_DEBUG_FMT("从环境变量加载日志文件路径: %s", log_path);
            }
        }

        if (const char* level = std::getenv(config::env::LOG_LEVEL)) {
            LogLevel logLevel;
            if (parseLogLevelString(level, logLevel)) {
                options.logLevel = logLevel;
                if (utils::Logger::isInitialized()) {
                    LOG_DEBUG_FMT("从环境变量加载日志级别: %s", level);
                }
            } else {
                if (utils::Logger::isInitialized()) {
                    LOG_WARNING_FMT("无效的环境变量日志级别: %s", level);
                }
            }
        }

        if (const char* format = std::getenv(config::env::REPORT_FORMAT)) {
            ReportFormat reportFormat;
            if (parseReportFormatString(format, reportFormat)) {
                options.reportFormat = reportFormat;
                if (utils::Logger::isInitialized()) {
                    LOG_DEBUG_FMT("从环境变量加载报告格式: %s", format);
                }
            } else {
                if (utils::Logger::isInitialized()) {
                    LOG_WARNING_FMT("无效的环境变量报告格式: %s", format);
                }
            }
        }

        if (const char* exclude = std::getenv(config::env::EXCLUDE)) {
            options.excludePatterns.clear();
            std::istringstream iss(exclude);
            std::string pattern;
            while (std::getline(iss, pattern, ',')) {
                if (!pattern.empty()) {
                    options.excludePatterns.push_back(pattern);
                    if (utils::Logger::isInitialized()) {
                        LOG_DEBUG_FMT("从环境变量加载排除模式: %s", pattern.c_str());
                    }
                }
            }
        }

        if (utils::Logger::isInitialized()) {
            LOG_INFO("成功从环境变量加载选项");
        }
        return true;
    } catch (const std::exception& e) {
        setError(ConfigError::EnvironmentError, e.what());
        return false;
    }
}

}  // namespace cli
}  // namespace dlogcover
