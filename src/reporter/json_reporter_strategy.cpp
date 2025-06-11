/**
 * @file json_reporter_strategy.cpp
 * @brief JSON报告生成策略实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/reporter/json_reporter_strategy.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/string_utils.h>

#include <algorithm>
#include <fstream>
#include <iomanip>

namespace dlogcover {
namespace reporter {

void JsonReporterStrategy::setConfig(const config::Config& config) {
    config_ = config;
    LOG_DEBUG("JSON报告策略配置已设置");
}

Result<bool> JsonReporterStrategy::generateReport(
    const std::string& outputPath,
    const core::coverage::CoverageStats& stats,
    const std::unordered_map<std::string, core::coverage::CoverageStats>& allStats,
    const ProgressCallback& progressCallback) {

    LOG_DEBUG_FMT("生成JSON报告: %s", outputPath.c_str());

    // 检查目录并创建
    size_t lastSlashPos = outputPath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        std::string dirPath = outputPath.substr(0, lastSlashPos);
        if (!dirPath.empty() && !createDirectoryIfNotExists(dirPath)) {
            LOG_ERROR_FMT("无法创建输出目录: %s", dirPath.c_str());
            return makeError<bool>(ReporterError::DIRECTORY_ERROR, "无法创建输出目录");
        }
    }

    // 进度回调
    if (progressCallback) {
        progressCallback(0.1f, "开始生成JSON报告");
    }

    try {
        // 创建根JSON对象
        nlohmann::json report;

        // 添加报告元数据
        report["metadata"] = {
            {"generator", "DLogCover"},
            {"version", "1.0.0"},
            {"date", getCurrentDateTimeString()},
            {"format", "json"}
        };

        // 进度回调
        if (progressCallback) {
            progressCallback(0.3f, "生成总体统计数据");
        }

        // 添加总体统计信息
        report["overall"] = createStatsJson(stats);

        // 进度回调
        if (progressCallback) {
            progressCallback(0.5f, "生成文件统计数据");
        }

        // 添加文件统计信息
        nlohmann::json filesJson = nlohmann::json::array();

        for (const auto& [filePath, fileStats] : allStats) {
            filesJson.push_back(createFileStatsJson(filePath, fileStats));
        }

        report["files"] = filesJson;

        // 进度回调
        if (progressCallback) {
            progressCallback(0.7f, "生成未覆盖路径数据");
        }

        // 添加未覆盖路径信息
        if (config_.output.show_uncovered_paths_details) {
            nlohmann::json uncoveredPathsJson = nlohmann::json::array();

            for (const auto& uncoveredPath : stats.uncoveredPaths) {
                uncoveredPathsJson.push_back(createUncoveredPathJson(uncoveredPath));
            }

            report["uncovered_paths"] = uncoveredPathsJson;
        }

        // 进度回调
        if (progressCallback) {
            progressCallback(0.9f, "生成建议数据");
        }

        // 添加改进建议
        report["suggestions"] = createSuggestionsJson(stats);

        // 写入文件
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            LOG_ERROR_FMT("无法打开输出文件: %s", outputPath.c_str());
            return makeError<bool>(ReporterError::FILE_ERROR, "无法打开输出文件: " + outputPath);
        }

        file << std::setw(2) << report << std::endl;
        file.close();

        // 完成回调
        if (progressCallback) {
            progressCallback(1.0f, "JSON报告生成完成");
        }

        LOG_INFO_FMT("JSON报告生成成功: %s", outputPath.c_str());
        return makeSuccess(true);
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("生成JSON报告时发生异常: %s", e.what());
        return makeError<bool>(ReporterError::GENERATION_ERROR,
                              std::string("生成JSON报告时发生异常: ") + e.what());
    }
}

nlohmann::json JsonReporterStrategy::createStatsJson(const core::coverage::CoverageStats& stats) const {
    return {
        {"function_coverage", {
            {"percentage", stats.functionCoverage * 100},
            {"covered", stats.coveredFunctions},
            {"total", stats.totalFunctions}
        }},
        {"branch_coverage", {
            {"percentage", stats.branchCoverage * 100},
            {"covered", stats.coveredBranches},
            {"total", stats.totalBranches}
        }},
        {"exception_coverage", {
            {"percentage", stats.exceptionCoverage * 100},
            {"covered", stats.coveredExceptionHandlers},
            {"total", stats.totalExceptionHandlers}
        }},
        {"key_path_coverage", {
            {"percentage", stats.keyPathCoverage * 100},
            {"covered", stats.coveredKeyPaths},
            {"total", stats.totalKeyPaths}
        }},
        {"overall_coverage", stats.overallCoverage * 100},
        {"uncovered_paths_count", stats.uncoveredPaths.size()}
    };
}

nlohmann::json JsonReporterStrategy::createFileStatsJson(const std::string& filePath,
                                                       const core::coverage::CoverageStats& stats) const {
    nlohmann::json fileJson;
    fileJson["path"] = dlogcover::utils::to_utf8(filePath);
    fileJson["function_coverage"] = {
        {"percentage", stats.functionCoverage * 100},
        {"covered", stats.coveredFunctions},
        {"total", stats.totalFunctions}
    };
    fileJson["branch_coverage"] = {
        {"percentage", stats.branchCoverage * 100},
        {"covered", stats.coveredBranches},
        {"total", stats.totalBranches}
    };
    fileJson["exception_coverage"] = {
        {"percentage", stats.exceptionCoverage * 100},
        {"covered", stats.coveredExceptionHandlers},
        {"total", stats.totalExceptionHandlers}
    };
    fileJson["key_path_coverage"] = {
        {"percentage", stats.keyPathCoverage * 100},
        {"covered", stats.coveredKeyPaths},
        {"total", stats.totalKeyPaths}
    };
    fileJson["overall_coverage"] = stats.overallCoverage * 100;
    fileJson["uncovered_paths_count"] = stats.uncoveredPaths.size();

    return fileJson;
}

nlohmann::json JsonReporterStrategy::createUncoveredPathJson(const core::coverage::UncoveredPathInfo& uncoveredPath) const {
    nlohmann::json pathJson;

    // 设置覆盖类型
    pathJson["type"] = [&]() {
        switch (uncoveredPath.type) {
            case core::coverage::CoverageType::FUNCTION:
                return "function";
            case core::coverage::CoverageType::BRANCH:
                return "branch";
            case core::coverage::CoverageType::EXCEPTION:
                return "exception";
            case core::coverage::CoverageType::KEY_PATH:
                return "key_path";
            default:
                return "unknown";
        }
    }();

    // 设置节点类型
    pathJson["node_type"] = [&]() {
        switch (uncoveredPath.nodeType) {
            case core::ast_analyzer::NodeType::FUNCTION:
                return "function";
            case core::ast_analyzer::NodeType::METHOD:
                return "method";
            case core::ast_analyzer::NodeType::IF_STMT:
                return "if_stmt";
            case core::ast_analyzer::NodeType::ELSE_STMT:
                return "else_stmt";
            case core::ast_analyzer::NodeType::SWITCH_STMT:
                return "switch_stmt";
            case core::ast_analyzer::NodeType::CASE_STMT:
                return "case_stmt";
            case core::ast_analyzer::NodeType::FOR_STMT:
                return "for_stmt";
            case core::ast_analyzer::NodeType::WHILE_STMT:
                return "while_stmt";
            case core::ast_analyzer::NodeType::DO_STMT:
                return "do_stmt";
            case core::ast_analyzer::NodeType::TRY_STMT:
                return "try_stmt";
            case core::ast_analyzer::NodeType::CATCH_STMT:
                return "catch_stmt";
            case core::ast_analyzer::NodeType::CALL_EXPR:
                return "call_expr";
            case core::ast_analyzer::NodeType::LOG_CALL_EXPR:
                return "log_call_expr";
            default:
                return "unknown";
        }
    }();

    // 设置位置信息
    pathJson["location"] = {
        {"file", dlogcover::utils::to_utf8(uncoveredPath.location.filePath)},
        {"line", uncoveredPath.location.line},
        {"column", uncoveredPath.location.column}
    };

    // 设置名称、文本和建议
    pathJson["name"] = dlogcover::utils::to_utf8(uncoveredPath.name);
    pathJson["text"] = dlogcover::utils::to_utf8(uncoveredPath.text);
    pathJson["suggestion"] = dlogcover::utils::to_utf8(uncoveredPath.suggestion);

    return pathJson;
}

nlohmann::json JsonReporterStrategy::createSuggestionsJson(const core::coverage::CoverageStats& stats) const {
    nlohmann::json suggestions;

    // 基础建议
    suggestions["general"] = "确保所有函数入口和关键分支都有适当级别的日志记录。";

    // 函数相关建议
    if (stats.functionCoverage < 0.7) {
        suggestions["functions"] = "对于未覆盖的函数，添加调试级别日志记录函数调用和参数。";
    } else {
        suggestions["functions"] = "函数覆盖率良好，继续保持良好的函数入口日志记录习惯。";
    }

    // 分支相关建议
    if (stats.branchCoverage < 0.7) {
        suggestions["branches"] = "对于未覆盖的分支，添加适当级别的日志记录执行路径和状态变化。";
    } else {
        suggestions["branches"] = "分支覆盖率良好，可以关注复杂条件分支的日志记录质量。";
    }

    // 异常相关建议
    if (stats.exceptionCoverage < 0.9) {
        suggestions["exceptions"] = "对于未覆盖的异常处理，添加警告或错误级别日志记录异常信息和处理方式。";
    } else {
        suggestions["exceptions"] = "异常覆盖率良好，确保异常处理中包含足够的上下文信息。";
    }

    // 关键路径相关建议
    if (stats.keyPathCoverage < 0.8) {
        suggestions["key_paths"] = "对于未覆盖的关键路径，添加信息级别日志记录重要操作和状态变化。";
    } else {
        suggestions["key_paths"] = "关键路径覆盖率良好，可以优化日志的信息含量。";
    }

    // 日志实践建议
    suggestions["best_practices"] = {
        "函数入口使用debug级别日志，记录函数名和关键参数",
        "重要业务逻辑使用info级别日志，记录操作和结果",
        "异常分支使用warning级别日志，记录异常情况",
        "错误处理使用error级别日志，记录错误信息和上下文",
        "致命错误使用fatal级别日志，记录导致程序终止的错误"
    };

    return suggestions;
}

std::string JsonReporterStrategy::getCurrentDateTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool JsonReporterStrategy::createDirectoryIfNotExists(const std::string& path) const {
    return utils::FileUtils::createDirectoryIfNotExists(path);
}

} // namespace reporter
} // namespace dlogcover
