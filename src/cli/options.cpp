/**
 * @file options.cpp
 * @brief 命令行选项实现
 * @copyright Copyright (c) 2023 DLogCover Team
 *
 * 该文件实现了命令行选项的验证和管理功能。主要包括：
 * - 选项有效性验证
 * - 选项值格式化
 * - 默认值管理
 *
 * 使用示例：
 * @code
 * Options options;
 * options.directory = "/path/to/scan";
 * if (auto result = options.validate(); result.hasError()) {
 *     std::cerr << "选项验证失败: " << result.message() << std::endl;
 *     return 1;
 * }
 * @endcode
 */

#include <dlogcover/cli/config_constants.h>
#include <dlogcover/cli/error_types.h>
#include <dlogcover/cli/options.h>
#include <dlogcover/common/log_types.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/string_utils.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace dlogcover {
namespace cli {

namespace {
/**
 * @brief 验证目录路径
 * @param path 目录路径
 * @return 错误结果
 */
ErrorResult validateDirectoryPath(std::string_view path) {
    LOG_DEBUG_FMT("验证目录路径: %.*s", static_cast<int>(path.size()), path.data());

    if (path.empty()) {
        return ErrorResult();  // 空路径允许使用默认值
    }

    std::filesystem::path fsPath(path);
    if (!std::filesystem::exists(fsPath)) {
        LOG_ERROR_FMT("目录不存在: %s", std::string(path).c_str());
        return ErrorResult(ConfigError::DirectoryNotFound,
                           std::string(config::error::DIRECTORY_NOT_FOUND) + std::string(path));
    }

    if (!std::filesystem::is_directory(fsPath)) {
        LOG_ERROR_FMT("路径不是目录: %s", std::string(path).c_str());
        return ErrorResult(ConfigError::DirectoryNotFound,
                           std::string(config::error::DIRECTORY_NOT_FOUND) + std::string(path));
    }

    return ErrorResult();
}

/**
 * @brief 验证文件路径
 * @param path 文件路径
 * @param checkParentDir 是否检查父目录存在性
 * @return 错误结果
 */
ErrorResult validateFilePath(std::string_view path, bool checkParentDir) {
    LOG_DEBUG_FMT("验证文件路径: %.*s, 检查父目录: %d", static_cast<int>(path.size()), path.data(), checkParentDir);

    if (path.empty()) {
        return ErrorResult();  // 空路径允许使用默认值
    }

    std::filesystem::path fsPath(path);

    if (checkParentDir) {
        auto parentPath = fsPath.parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
            LOG_ERROR_FMT("父目录不存在: %s", parentPath.string().c_str());
            return ErrorResult(ConfigError::OutputDirectoryNotFound,
                               std::string(config::error::OUTPUT_DIR_NOT_FOUND) + parentPath.string());
        }
    } else {
        if (!std::filesystem::exists(fsPath)) {
            LOG_ERROR_FMT("文件不存在: %s", std::string(path).c_str());
            return ErrorResult(ConfigError::FileNotFound,
                               std::string(config::error::FILE_NOT_FOUND) + std::string(path));
        }
    }

    return ErrorResult();
}

// 移除未使用的 toStdString 函数
}  // namespace

std::string_view toString(LogLevel level) {
    // 使用统一的toString函数
    return common::toString(level);
}

LogLevel parseLogLevel(std::string_view str) {
    // 使用统一的parseLogLevel函数
    return common::parseLogLevel(str);
}

std::string_view toString(ReportFormat format) {
    switch (format) {
        case ReportFormat::TEXT:
            return config::report::TEXT;
        case ReportFormat::JSON:
            return config::report::JSON;
        default:
            return config::report::TEXT;
    }
}

ReportFormat parseReportFormat(std::string_view str) {
    std::string strLower{str};
    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);

    if (strLower == config::report::TEXT)
        return ReportFormat::TEXT;
    else if (strLower == config::report::JSON)
        return ReportFormat::JSON;

    throw std::invalid_argument(std::string(config::error::INVALID_REPORT_FORMAT) + std::string(str));
}

ErrorResult Options::validate() const {
    // 验证include模式
    for (const auto& pattern : includePatterns) {
        if (pattern.empty()) {
            LOG_WARNING("发现空的include模式");
        }
    }
    LOG_DEBUG("开始验证选项");

    // 验证扫描目录
    if (auto result = validateDirectoryPath(directory); result.hasError()) {
        return result;
    }

    // 验证配置文件
    if (auto result = validateFilePath(configPath, false); result.hasError()) {
        return result;
    }

    // 验证输出路径
    if (auto result = validateFilePath(output_file, true); result.hasError()) {
        return result;
    }

    // 验证排除模式
    for (const auto& pattern : excludePatterns) {
        if (pattern.empty()) {
            LOG_ERROR("发现空的排除模式");
            return ErrorResult(ConfigError::InvalidExcludePattern, std::string(config::error::INVALID_EXCLUDE));
        }
    }

    LOG_DEBUG("选项验证通过");
    return ErrorResult();
}

std::string Options::toString() const {
    std::ostringstream oss;
    oss << "Options {\n"
        << "  directory: " << directory << "\n"
        << "  output_file: " << output_file << "\n"
        << "  configPath: " << configPath << "\n"
        << "  excludePatterns: [";

    for (size_t i = 0; i < excludePatterns.size(); ++i) {
        if (i > 0)
            oss << ", ";
        oss << excludePatterns[i];
    }
    oss << "]\n"
        << "  includePatterns: [";
    for (size_t i = 0; i < includePatterns.size(); ++i) {
        if (i > 0)
            oss << ", ";
        oss << includePatterns[i];
    }
    oss << "]\n"
        << "  logLevel: " << cli::toString(logLevel) << "\n"
        << "  reportFormat: " << cli::toString(reportFormat) << "\n"
        << "}";

    return oss.str();
}

void Options::reset() {
    LOG_DEBUG("重置选项为默认值");

    directory = std::string(config::cli::DEFAULT_DIRECTORY);
    output_file = std::string(config::cli::DEFAULT_OUTPUT);
    configPath = std::string(config::cli::DEFAULT_CONFIG);
    excludePatterns.clear();
    logLevel = LogLevel::ALL;
    reportFormat = ReportFormat::TEXT;
}

std::string Options::toJson() const {
    LOG_DEBUG("序列化选项为JSON");

    nlohmann::json j;
    j["directory"] = directory;
    j["output"] = output_file;
    j["config"] = configPath;
    j["exclude"] = excludePatterns;
    j["include"] = includePatterns; // 新增序列化include_patterns
    j["log_level"] = std::string(cli::toString(logLevel));
    j["report_format"] = std::string(cli::toString(reportFormat));
    j["version"] = config::CURRENT_CONFIG_VERSION;

    return j.dump(2);  // 使用2空格缩进
}

ErrorResult Options::fromJson(std::string_view json) {
    LOG_DEBUG("从JSON反序列化选项");

    try {
        auto j = nlohmann::json::parse(json);

        // 验证版本
        if (j.find("version") == j.end()) {
            return ErrorResult(ConfigError::InvalidVersion,
                               std::string(config::error::INVALID_VERSION) + "missing version field");
        }

        std::string version = j["version"];
        if (version != config::CURRENT_CONFIG_VERSION) {
            return ErrorResult(ConfigError::InvalidVersion, std::string(config::error::INVALID_VERSION) + version);
        }

        // 必需字段
        if (j.find("directory") == j.end()) {
            return ErrorResult(ConfigError::MissingField, std::string(config::error::MISSING_FIELD) + "directory");
        }

        // 读取字段
        directory = j["directory"];
        output_file = j.value("output", std::string(config::cli::DEFAULT_OUTPUT));
        configPath = j.value("config", std::string(config::cli::DEFAULT_CONFIG));
        excludePatterns = j.value("exclude", std::vector<std::string>());
        includePatterns = j.value("include", std::vector<std::string>());

        // 解析日志级别
        if (j.find("log_level") != j.end()) {
            try {
                logLevel = parseLogLevel(j["log_level"]);
            } catch (const std::invalid_argument& e) {
                return ErrorResult(ConfigError::InvalidLogLevel, e.what());
            }
        }

        // 解析报告格式
        if (j.find("report_format") != j.end()) {
            try {
                reportFormat = parseReportFormat(j["report_format"]);
            } catch (const std::invalid_argument& e) {
                return ErrorResult(ConfigError::InvalidReportFormat, e.what());
            }
        }

        return ErrorResult();
    } catch (const nlohmann::json::exception& e) {
        return ErrorResult(ConfigError::ParseError, std::string(config::error::PARSE_ERROR) + e.what());
    }
}

bool Options::isValid() const {
    return !validate().hasError();
}

bool Options::operator==(const Options& other) const {
    return directory == other.directory && output_file == other.output_file && configPath == other.configPath &&
           excludePatterns == other.excludePatterns && logLevel == other.logLevel && reportFormat == other.reportFormat;
}

bool Options::operator!=(const Options& other) const {
    return !(*this == other);
}

}  // namespace cli
}  // namespace dlogcover
