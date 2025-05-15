/**
 * @file reporter.cpp
 * @brief 报告生成器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/reporter/reporter.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <chrono>
#include <ctime>
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

    // 根据配置选择报告格式
    if (config_.report.format == "json") {
        return generateJsonReport(outputPath);
    } else {
        return generateTextReport(outputPath);
    }
}

bool Reporter::generateTextReport(const std::string& outputPath) {
    LOG_INFO("生成文本格式报告");

    // 创建输出目录（如果不存在）
    std::string outputDir = utils::FileUtils::getDirectoryName(outputPath);
    if (!outputDir.empty() && !utils::FileUtils::directoryExists(outputDir)) {
        if (!utils::FileUtils::createDirectory(outputDir)) {
            LOG_ERROR_FMT("无法创建输出目录: %s", outputDir.c_str());
            return false;
        }
    }

    // 打开输出文件
    std::ofstream outFile(outputPath);
    if (!outFile) {
        LOG_ERROR_FMT("无法打开输出文件: %s", outputPath.c_str());
        return false;
    }

    // 获取总体覆盖率统计
    const core::coverage::CoverageStats& overallStats = coverageCalculator_.getOverallCoverageStats();

    // 生成报告标题
    outFile << "# DLogCover 日志覆盖率报告\n\n";
    outFile << "生成时间: " << getCurrentDateTimeString() << "\n\n";

    // 生成总体统计部分
    outFile << "## 总体覆盖率\n\n";
    outFile << generateOverallStatsText(overallStats);
    outFile << "\n";

    // 生成文件级覆盖率统计
    outFile << "## 文件覆盖率\n\n";

    const auto& allStats = coverageCalculator_.getAllCoverageStats();
    if (allStats.empty()) {
        outFile << "未分析任何文件。\n\n";
    } else {
        outFile << "| 文件 | 函数覆盖率 | 分支覆盖率 | 异常覆盖率 | 总体覆盖率 |\n";
        outFile << "|------|------------|------------|------------|------------|\n";

        for (const auto& [filePath, stats] : allStats) {
            std::string fileDisplayName = utils::FileUtils::getFileName(filePath);

            outFile << "| " << fileDisplayName << " | ";
            outFile << std::fixed << std::setprecision(1) << (stats.functionCoverage * 100.0) << "% | ";
            outFile << std::fixed << std::setprecision(1) << (stats.branchCoverage * 100.0) << "% | ";
            outFile << std::fixed << std::setprecision(1) << (stats.exceptionCoverage * 100.0) << "% | ";
            outFile << std::fixed << std::setprecision(1) << (stats.overallCoverage * 100.0) << "% |\n";
        }

        outFile << "\n";
    }

    // 生成未覆盖路径部分
    if (!overallStats.uncoveredPaths.empty()) {
        outFile << "## 未覆盖路径\n\n";

        for (size_t i = 0; i < overallStats.uncoveredPaths.size(); ++i) {
            const auto& path = overallStats.uncoveredPaths[i];
            outFile << "### " << (i + 1) << ". " << utils::FileUtils::getFileName(path.location.filePath) << ":"
                    << path.location.line << "\n\n";
            outFile << generateUncoveredPathText(path);
            outFile << "\n";
        }
    }

    // 生成建议部分
    outFile << "## 改进建议\n\n";
    outFile << generateSuggestionsText(overallStats);

    outFile.close();
    LOG_INFO("文本报告生成完成");
    return true;
}

bool Reporter::generateJsonReport(const std::string& outputPath) {
    LOG_INFO("生成JSON格式报告");

    // 创建输出目录（如果不存在）
    std::string outputDir = utils::FileUtils::getDirectoryName(outputPath);
    if (!outputDir.empty() && !utils::FileUtils::directoryExists(outputDir)) {
        if (!utils::FileUtils::createDirectory(outputDir)) {
            LOG_ERROR_FMT("无法创建输出目录: %s", outputDir.c_str());
            return false;
        }
    }

    // 打开输出文件
    std::ofstream outFile(outputPath);
    if (!outFile) {
        LOG_ERROR_FMT("无法打开输出文件: %s", outputPath.c_str());
        return false;
    }

    // 获取总体覆盖率统计
    const core::coverage::CoverageStats& overallStats = coverageCalculator_.getOverallCoverageStats();

    // 创建JSON对象
    nlohmann::json reportJson;

    // 添加元数据
    reportJson["metadata"]["timestamp"] = getCurrentDateTimeString();
    reportJson["metadata"]["format"] = "json";

    // 添加总体统计
    reportJson["overall"]["function_coverage"] = overallStats.functionCoverage;
    reportJson["overall"]["branch_coverage"] = overallStats.branchCoverage;
    reportJson["overall"]["exception_coverage"] = overallStats.exceptionCoverage;
    reportJson["overall"]["key_path_coverage"] = overallStats.keyPathCoverage;
    reportJson["overall"]["total_coverage"] = overallStats.overallCoverage;

    reportJson["overall"]["total_functions"] = overallStats.totalFunctions;
    reportJson["overall"]["covered_functions"] = overallStats.coveredFunctions;

    reportJson["overall"]["total_branches"] = overallStats.totalBranches;
    reportJson["overall"]["covered_branches"] = overallStats.coveredBranches;

    reportJson["overall"]["total_exception_handlers"] = overallStats.totalExceptionHandlers;
    reportJson["overall"]["covered_exception_handlers"] = overallStats.coveredExceptionHandlers;

    // 添加文件级统计
    const auto& allStats = coverageCalculator_.getAllCoverageStats();
    for (const auto& [filePath, stats] : allStats) {
        nlohmann::json fileJson;

        fileJson["path"] = filePath;
        fileJson["function_coverage"] = stats.functionCoverage;
        fileJson["branch_coverage"] = stats.branchCoverage;
        fileJson["exception_coverage"] = stats.exceptionCoverage;
        fileJson["key_path_coverage"] = stats.keyPathCoverage;
        fileJson["total_coverage"] = stats.overallCoverage;

        fileJson["total_functions"] = stats.totalFunctions;
        fileJson["covered_functions"] = stats.coveredFunctions;

        fileJson["total_branches"] = stats.totalBranches;
        fileJson["covered_branches"] = stats.coveredBranches;

        fileJson["total_exception_handlers"] = stats.totalExceptionHandlers;
        fileJson["covered_exception_handlers"] = stats.coveredExceptionHandlers;

        reportJson["files"].push_back(fileJson);
    }

    // 添加未覆盖路径
    for (const auto& path : overallStats.uncoveredPaths) {
        nlohmann::json uncoveredPathJson;

        switch (path.type) {
            case core::coverage::CoverageType::FUNCTION:
                uncoveredPathJson["type"] = "function";
                break;
            case core::coverage::CoverageType::BRANCH:
                uncoveredPathJson["type"] = "branch";
                break;
            case core::coverage::CoverageType::EXCEPTION:
                uncoveredPathJson["type"] = "exception";
                break;
            case core::coverage::CoverageType::KEY_PATH:
                uncoveredPathJson["type"] = "key_path";
                break;
        }

        uncoveredPathJson["file"] = path.location.filePath;
        uncoveredPathJson["line"] = path.location.line;
        uncoveredPathJson["column"] = path.location.column;
        uncoveredPathJson["name"] = path.name;
        uncoveredPathJson["text"] = path.text;
        uncoveredPathJson["suggestion"] = path.suggestion;

        reportJson["uncovered_paths"].push_back(uncoveredPathJson);
    }

    // 写入JSON数据
    outFile << std::setw(4) << reportJson << std::endl;

    outFile.close();
    LOG_INFO("JSON报告生成完成");
    return true;
}

std::string Reporter::generateOverallStatsText(const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 输出总体统计信息
    ss << "- 函数覆盖率: " << std::fixed << std::setprecision(1) << (stats.functionCoverage * 100.0) << "% ("
       << stats.coveredFunctions << "/" << stats.totalFunctions << ")\n";

    ss << "- 分支覆盖率: " << std::fixed << std::setprecision(1) << (stats.branchCoverage * 100.0) << "% ("
       << stats.coveredBranches << "/" << stats.totalBranches << ")\n";

    ss << "- 异常覆盖率: " << std::fixed << std::setprecision(1) << (stats.exceptionCoverage * 100.0) << "% ("
       << stats.coveredExceptionHandlers << "/" << stats.totalExceptionHandlers << ")\n";

    ss << "- 关键路径覆盖率: " << std::fixed << std::setprecision(1) << (stats.keyPathCoverage * 100.0) << "%\n";

    ss << "- 总体覆盖率: " << std::fixed << std::setprecision(1) << (stats.overallCoverage * 100.0) << "%\n";

    return ss.str();
}

std::string Reporter::generateFileStatsText(const std::string& filePath,
                                            const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 输出文件名
    ss << "### " << utils::FileUtils::getFileName(filePath) << "\n\n";

    // 输出该文件的统计信息
    ss << "- 函数覆盖率: " << std::fixed << std::setprecision(1) << (stats.functionCoverage * 100.0) << "% ("
       << stats.coveredFunctions << "/" << stats.totalFunctions << ")\n";

    ss << "- 分支覆盖率: " << std::fixed << std::setprecision(1) << (stats.branchCoverage * 100.0) << "% ("
       << stats.coveredBranches << "/" << stats.totalBranches << ")\n";

    ss << "- 异常覆盖率: " << std::fixed << std::setprecision(1) << (stats.exceptionCoverage * 100.0) << "% ("
       << stats.coveredExceptionHandlers << "/" << stats.totalExceptionHandlers << ")\n";

    ss << "- 总体覆盖率: " << std::fixed << std::setprecision(1) << (stats.overallCoverage * 100.0) << "%\n";

    return ss.str();
}

std::string Reporter::generateUncoveredPathText(const core::coverage::UncoveredPathInfo& uncoveredPath) const {
    std::stringstream ss;

    // 输出未覆盖路径信息
    ss << "- 位置: " << uncoveredPath.location.filePath << ":" << uncoveredPath.location.line << ":"
       << uncoveredPath.location.column << "\n";

    ss << "- 名称: " << uncoveredPath.name << "\n";

    std::string typeStr;
    switch (uncoveredPath.type) {
        case core::coverage::CoverageType::FUNCTION:
            typeStr = "函数";
            break;
        case core::coverage::CoverageType::BRANCH:
            typeStr = "分支";
            break;
        case core::coverage::CoverageType::EXCEPTION:
            typeStr = "异常处理";
            break;
        case core::coverage::CoverageType::KEY_PATH:
            typeStr = "关键路径";
            break;
    }
    ss << "- 类型: " << typeStr << "\n";

    if (!uncoveredPath.text.empty()) {
        ss << "- 代码片段:\n```cpp\n" << uncoveredPath.text << "\n```\n";
    }

    if (!uncoveredPath.suggestion.empty()) {
        ss << "- 建议: " << uncoveredPath.suggestion << "\n";
    }

    return ss.str();
}

std::string Reporter::generateSuggestionsText(const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 输出总体建议
    if (stats.overallCoverage >= 0.9) {
        ss << "### 覆盖率等级: 优秀\n\n";
        ss << "- 您的项目日志覆盖率非常好，几乎所有重要代码路径都有日志覆盖。\n";
        ss << "- 继续保持这种良好的日志记录习惯。\n";
    } else if (stats.overallCoverage >= 0.75) {
        ss << "### 覆盖率等级: 良好\n\n";
        ss << "- 您的项目日志覆盖率良好，大多数重要代码路径有日志覆盖。\n";
        ss << "- 建议关注那些未被覆盖的部分，特别是异常处理相关代码。\n";
    } else if (stats.overallCoverage >= 0.5) {
        ss << "### 覆盖率等级: 一般\n\n";
        ss << "- 您的项目日志覆盖率一般，主要代码路径有日志覆盖，但有改进空间。\n";
        ss << "- 建议增加函数入口和关键分支的日志覆盖。\n";
        ss << "- 重点关注异常处理和错误处理代码的日志覆盖。\n";
    } else {
        ss << "### 覆盖率等级: 不足\n\n";
        ss << "- 您的项目日志覆盖率不足，许多重要代码路径缺少日志。\n";
        ss << "- 建议建立日志规范，要求函数入口和重要返回点必须添加日志。\n";
        ss << "- 所有异常处理和错误处理代码必须添加警告或错误级别日志。\n";
        ss << "- 关键业务流程需要添加信息级别日志。\n";
    }

    // 输出具体建议
    ss << "\n### 具体建议\n\n";

    if (stats.functionCoverage < 0.7) {
        ss << "- **函数覆盖率不足**: 建议所有公共函数和方法入口添加调试级别日志，关键函数出口添加信息级别日志。\n";
    }

    if (stats.branchCoverage < 0.7) {
        ss << "- **分支覆盖率不足**: 建议在所有重要条件分支中添加适当级别的日志，特别是错误处理分支。\n";
    }

    if (stats.exceptionCoverage < 0.8) {
        ss << "- **异常覆盖率不足**: 建议所有catch块中添加警告或错误级别日志，详细记录异常信息。\n";
    }

    if (stats.keyPathCoverage < 0.8) {
        ss << "- **关键路径覆盖率不足**: 建议识别应用程序的关键执行路径，并在路径的重要节点添加信息级别日志。\n";
    }

    return ss.str();
}

std::string Reporter::getCurrentDateTimeString() const {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // 格式化时间戳
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

}  // namespace reporter
}  // namespace dlogcover
