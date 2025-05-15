/**
 * @file reporter.cpp
 * @brief 报告生成器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/reporter/reporter.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace dlogcover {
namespace reporter {

Reporter::Reporter(const config::Config& config, const core::coverage::CoverageCalculator& coverageCalculator)
    : config_(config), coverageCalculator_(coverageCalculator) {
    LOG_DEBUG("报告生成器初始化");
}

Reporter::~Reporter() {
    LOG_DEBUG("报告生成器销毁");
}

bool Reporter::generateReport(const std::string& outputPath) {
    LOG_INFO_FMT("生成报告: %s", outputPath.c_str());

    // 根据配置的格式生成不同类型的报告
    bool success = false;
    if (config_.report.format == "json") {
        success = generateJsonReport(outputPath);
    } else {
        // 默认使用文本格式
        success = generateTextReport(outputPath);
    }

    if (success) {
        LOG_INFO_FMT("报告生成成功: %s", outputPath.c_str());
    } else {
        LOG_ERROR_FMT("报告生成失败: %s", outputPath.c_str());
    }

    return success;
}

bool Reporter::generateTextReport(const std::string& outputPath) {
    LOG_DEBUG_FMT("生成文本报告: %s", outputPath.c_str());

    std::ofstream file(outputPath);
    if (!file.is_open()) {
        LOG_ERROR_FMT("无法打开输出文件: %s", outputPath.c_str());
        return false;
    }

    // 获取总体覆盖率统计信息
    const auto& overallStats = coverageCalculator_.getOverallCoverageStats();

    // 生成报告头部
    file << "=====================================================" << std::endl;
    file << "            DLogCover 日志覆盖率报告                " << std::endl;
    file << "=====================================================" << std::endl;
    file << "生成时间: " << getCurrentDateTimeString() << std::endl;
    file << "----------------------------------------------------" << std::endl << std::endl;

    // 生成总体统计信息
    file << "总体覆盖率统计" << std::endl;
    file << "----------------------------------------------------" << std::endl;
    file << generateOverallStatsText(overallStats) << std::endl;

    // 分隔线
    file << std::endl << "=====================================================" << std::endl << std::endl;

    // 获取所有文件的覆盖率统计信息
    const auto& allStats = coverageCalculator_.getAllCoverageStats();

    // 按文件名排序
    std::vector<std::string> fileNames;
    for (const auto& [filePath, _] : allStats) {
        fileNames.push_back(filePath);
    }

    std::sort(fileNames.begin(), fileNames.end());

    // 生成每个文件的覆盖率统计
    file << "文件覆盖率统计" << std::endl;
    file << "----------------------------------------------------" << std::endl;

    for (const auto& filePath : fileNames) {
        const auto& stats = allStats.at(filePath);
        file << generateFileStatsText(filePath, stats) << std::endl;
        file << "----------------------------------------------------" << std::endl;
    }

    // 生成未覆盖路径信息
    if (!overallStats.uncoveredPaths.empty()) {
        file << std::endl << "未覆盖路径列表" << std::endl;
        file << "----------------------------------------------------" << std::endl;

        for (const auto& uncoveredPath : overallStats.uncoveredPaths) {
            file << generateUncoveredPathText(uncoveredPath) << std::endl;
        }
    }

    // 生成改进建议
    file << std::endl << "改进建议" << std::endl;
    file << "----------------------------------------------------" << std::endl;
    file << generateSuggestionsText(overallStats) << std::endl;

    // 报告尾部
    file << std::endl << "=====================================================" << std::endl;
    file << "               报告结束                             " << std::endl;
    file << "=====================================================" << std::endl;

    file.close();
    return true;
}

bool Reporter::generateJsonReport(const std::string& outputPath) {
    LOG_DEBUG_FMT("生成JSON报告: %s", outputPath.c_str());

    // 创建JSON对象
    nlohmann::json report;

    // 添加报告元数据
    report["metadata"] = {
        {"generator", "DLogCover"}, {"version", "1.0.0"}, {"date", getCurrentDateTimeString()}, {"format", "json"}};

    // 获取总体覆盖率统计信息
    const auto& overallStats = coverageCalculator_.getOverallCoverageStats();

    // 添加总体统计信息
    report["overall"] = {{"function_coverage",
                          {{"percentage", overallStats.functionCoverage * 100},
                           {"covered", overallStats.coveredFunctions},
                           {"total", overallStats.totalFunctions}}},
                         {"branch_coverage",
                          {{"percentage", overallStats.branchCoverage * 100},
                           {"covered", overallStats.coveredBranches},
                           {"total", overallStats.totalBranches}}},
                         {"exception_coverage",
                          {{"percentage", overallStats.exceptionCoverage * 100},
                           {"covered", overallStats.coveredExceptionHandlers},
                           {"total", overallStats.totalExceptionHandlers}}},
                         {"key_path_coverage",
                          {{"percentage", overallStats.keyPathCoverage * 100},
                           {"covered", overallStats.coveredKeyPaths},
                           {"total", overallStats.totalKeyPaths}}},
                         {"overall_coverage", overallStats.overallCoverage * 100},
                         {"total_coverage", overallStats.overallCoverage * 100}};

    // 获取所有文件的覆盖率统计信息
    const auto& allStats = coverageCalculator_.getAllCoverageStats();

    // 添加文件统计信息
    nlohmann::json filesJson = nlohmann::json::array();

    for (const auto& [filePath, stats] : allStats) {
        nlohmann::json fileJson;
        fileJson["path"] = filePath;
        fileJson["function_coverage"] = {{"percentage", stats.functionCoverage * 100},
                                         {"covered", stats.coveredFunctions},
                                         {"total", stats.totalFunctions}};
        fileJson["branch_coverage"] = {{"percentage", stats.branchCoverage * 100},
                                       {"covered", stats.coveredBranches},
                                       {"total", stats.totalBranches}};
        fileJson["exception_coverage"] = {{"percentage", stats.exceptionCoverage * 100},
                                          {"covered", stats.coveredExceptionHandlers},
                                          {"total", stats.totalExceptionHandlers}};
        fileJson["key_path_coverage"] = {{"percentage", stats.keyPathCoverage * 100},
                                         {"covered", stats.coveredKeyPaths},
                                         {"total", stats.totalKeyPaths}};
        fileJson["overall_coverage"] = stats.overallCoverage * 100;

        filesJson.push_back(fileJson);
    }

    report["files"] = filesJson;

    // 添加未覆盖路径信息
    nlohmann::json uncoveredPathsJson = nlohmann::json::array();

    for (const auto& uncoveredPath : overallStats.uncoveredPaths) {
        nlohmann::json pathJson;
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
        pathJson["location"] = {{"file", uncoveredPath.location.filePath},
                                {"line", uncoveredPath.location.line},
                                {"column", uncoveredPath.location.column}};
        pathJson["name"] = uncoveredPath.name;
        pathJson["text"] = uncoveredPath.text;
        pathJson["suggestion"] = uncoveredPath.suggestion;

        uncoveredPathsJson.push_back(pathJson);
    }

    report["uncovered_paths"] = uncoveredPathsJson;

    // 添加改进建议
    report["suggestions"] = {{"general", "确保所有函数入口和关键分支都有适当级别的日志记录。"},
                             {"functions", "对于未覆盖的函数，添加调试级别日志记录函数调用和参数。"},
                             {"branches", "对于未覆盖的分支，添加适当级别的日志记录执行路径和状态变化。"},
                             {"exceptions", "对于未覆盖的异常处理，添加警告或错误级别日志记录异常信息和处理方式。"},
                             {"key_paths", "对于未覆盖的关键路径，添加信息级别日志记录重要操作和状态变化。"}};

    // 写入文件
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        LOG_ERROR_FMT("无法打开输出文件: %s", outputPath.c_str());
        return false;
    }

    file << std::setw(2) << report << std::endl;
    file.close();

    return true;
}

std::string Reporter::generateOverallStatsText(const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    ss << "函数覆盖率: " << std::fixed << std::setprecision(2) << (stats.functionCoverage * 100) << "% ("
       << stats.coveredFunctions << "/" << stats.totalFunctions << ")" << std::endl;

    ss << "分支覆盖率: " << std::fixed << std::setprecision(2) << (stats.branchCoverage * 100) << "% ("
       << stats.coveredBranches << "/" << stats.totalBranches << ")" << std::endl;

    ss << "异常覆盖率: " << std::fixed << std::setprecision(2) << (stats.exceptionCoverage * 100) << "% ("
       << stats.coveredExceptionHandlers << "/" << stats.totalExceptionHandlers << ")" << std::endl;

    ss << "关键路径覆盖率: " << std::fixed << std::setprecision(2) << (stats.keyPathCoverage * 100) << "% ("
       << stats.coveredKeyPaths << "/" << stats.totalKeyPaths << ")" << std::endl;

    ss << "总体覆盖率: " << std::fixed << std::setprecision(2) << (stats.overallCoverage * 100) << "%";

    return ss.str();
}

std::string Reporter::generateFileStatsText(const std::string& filePath,
                                            const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 提取短文件名
    std::string shortFilePath = filePath;
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        shortFilePath = filePath.substr(lastSlash + 1);
    }

    ss << "文件: " << filePath << std::endl;

    if (stats.totalFunctions > 0) {
        ss << "  函数覆盖率: " << std::fixed << std::setprecision(2) << (stats.functionCoverage * 100) << "% ("
           << stats.coveredFunctions << "/" << stats.totalFunctions << ")" << std::endl;
    } else {
        ss << "  函数覆盖率: N/A (0/0)" << std::endl;
    }

    if (stats.totalBranches > 0) {
        ss << "  分支覆盖率: " << std::fixed << std::setprecision(2) << (stats.branchCoverage * 100) << "% ("
           << stats.coveredBranches << "/" << stats.totalBranches << ")" << std::endl;
    } else {
        ss << "  分支覆盖率: N/A (0/0)" << std::endl;
    }

    if (stats.totalExceptionHandlers > 0) {
        ss << "  异常覆盖率: " << std::fixed << std::setprecision(2) << (stats.exceptionCoverage * 100) << "% ("
           << stats.coveredExceptionHandlers << "/" << stats.totalExceptionHandlers << ")" << std::endl;
    } else {
        ss << "  异常覆盖率: N/A (0/0)" << std::endl;
    }

    if (stats.totalKeyPaths > 0) {
        ss << "  关键路径覆盖率: " << std::fixed << std::setprecision(2) << (stats.keyPathCoverage * 100) << "% ("
           << stats.coveredKeyPaths << "/" << stats.totalKeyPaths << ")" << std::endl;
    } else {
        ss << "  关键路径覆盖率: N/A (0/0)" << std::endl;
    }

    ss << "  总体覆盖率: " << std::fixed << std::setprecision(2) << (stats.overallCoverage * 100) << "%";

    // 添加未覆盖路径摘要
    if (!stats.uncoveredPaths.empty()) {
        ss << std::endl << "  未覆盖路径: " << stats.uncoveredPaths.size() << " 个";
    }

    return ss.str();
}

std::string Reporter::generateUncoveredPathText(const core::coverage::UncoveredPathInfo& uncoveredPath) const {
    std::stringstream ss;

    // 确定覆盖率类型文本
    std::string typeText;
    switch (uncoveredPath.type) {
        case core::coverage::CoverageType::FUNCTION:
            typeText = "函数";
            break;
        case core::coverage::CoverageType::BRANCH:
            typeText = "分支";
            break;
        case core::coverage::CoverageType::EXCEPTION:
            typeText = "异常";
            break;
        case core::coverage::CoverageType::KEY_PATH:
            typeText = "关键路径";
            break;
        default:
            typeText = "未知";
            break;
    }

    // 提取短文件名
    std::string shortFilePath = uncoveredPath.location.filePath;
    size_t lastSlash = shortFilePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        shortFilePath = shortFilePath.substr(lastSlash + 1);
    }

    ss << "[" << typeText << "] " << uncoveredPath.name << std::endl;
    ss << "  位置: " << uncoveredPath.location.filePath << ":" << uncoveredPath.location.line << ":"
       << uncoveredPath.location.column << std::endl;

    // 添加代码片段（如果有）
    if (!uncoveredPath.text.empty()) {
        ss << "  代码: " << uncoveredPath.text << std::endl;
    }

    // 添加建议
    ss << "  建议: " << uncoveredPath.suggestion;

    return ss.str();
}

std::string Reporter::generateSuggestionsText(const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 基础建议
    ss << "1. 确保所有函数入口和关键分支都有适当级别的日志记录。" << std::endl;

    // 根据总体覆盖率情况提供不同的建议
    if (stats.overallCoverage < 0.5) {
        ss << "2. 当前日志覆盖率较低，应优先关注函数入口和异常处理的日志记录。" << std::endl;
        ss << "3. 为主要功能模块添加详细的日志记录，便于问题定位。" << std::endl;
    } else if (stats.overallCoverage < 0.8) {
        ss << "2. 当前日志覆盖率中等，应关注关键分支和复杂逻辑的日志记录。" << std::endl;
        ss << "3. 完善错误处理路径的日志记录，提高问题定位能力。" << std::endl;
    } else {
        ss << "2. 当前日志覆盖率较高，可进一步优化日志的质量和详细程度。" << std::endl;
        ss << "3. 检查关键业务逻辑的日志级别是否合适。" << std::endl;
    }

    // 根据具体覆盖率类型提供针对性建议
    if (stats.functionCoverage < 0.7) {
        ss << "4. 函数级覆盖率较低，建议在函数入口添加debug级别日志，记录函数调用及参数。" << std::endl;
    }

    if (stats.branchCoverage < 0.7) {
        ss << "5. 分支覆盖率较低，建议在关键条件分支添加合适级别的日志，记录分支执行情况。" << std::endl;
    }

    if (stats.exceptionCoverage < 0.9) {
        ss << "6. 异常覆盖率不足，建议在所有异常处理块中添加warning或error级别日志。" << std::endl;
    }

    if (!stats.uncoveredPaths.empty()) {
        ss << "7. 有 " << stats.uncoveredPaths.size() << " 个未覆盖路径需要添加日志记录，请参考上述未覆盖路径列表。"
           << std::endl;
    }

    // 日志记录最佳实践
    ss << std::endl << "日志记录最佳实践：" << std::endl;
    ss << "- 函数入口应使用debug级别日志，记录函数名和关键参数" << std::endl;
    ss << "- 重要业务逻辑应使用info级别日志，记录操作和结果" << std::endl;
    ss << "- 异常分支应使用warning级别日志，记录异常情况" << std::endl;
    ss << "- 错误处理应使用error级别日志，记录错误信息和上下文" << std::endl;
    ss << "- 致命错误应使用fatal级别日志，记录导致程序终止的错误";

    return ss.str();
}

std::string Reporter::getCurrentDateTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

}  // namespace reporter
}  // namespace dlogcover
