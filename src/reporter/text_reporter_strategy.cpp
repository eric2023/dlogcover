/**
 * @file text_reporter_strategy.cpp
 * @brief 文本报告生成策略实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/reporter/text_reporter_strategy.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <locale>

namespace dlogcover {
namespace reporter {

Result<bool> TextReporterStrategy::generateReport(
    const std::string& outputPath, const core::coverage::CoverageStats& stats,
    const std::unordered_map<std::string, core::coverage::CoverageStats>& allStats,
    const ProgressCallback& progressCallback) {
    LOG_DEBUG_FMT("生成文本报告: %s", outputPath.c_str());

    // 检查目录并创建
    size_t lastSlashPos = outputPath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        std::string dirPath = outputPath.substr(0, lastSlashPos);
        if (!dirPath.empty() && !createDirectoryIfNotExists(dirPath)) {
            LOG_ERROR_FMT("无法创建输出目录: %s", dirPath.c_str());
            return makeError<bool>(ReporterError::DIRECTORY_ERROR, "无法创建输出目录");
        }
    }

    // 打开输出文件
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        LOG_ERROR_FMT("无法打开输出文件: %s", outputPath.c_str());
        return makeError<bool>(ReporterError::FILE_ERROR, "无法打开输出文件: " + outputPath);
    }

    // 设置UTF-8编码
    try {
        file.imbue(std::locale("en_US.UTF-8"));
    } catch (const std::runtime_error& e) {
        // 如果系统不支持en_US.UTF-8，则回退到默认locale，并记录警告
        LOG_WARNING_FMT("无法设置en_US.UTF-8编码，使用默认locale: %s", e.what());
    }

    // 进度回调
    if (progressCallback) {
        progressCallback(0.1f, "开始生成文本报告");
    }

    try {
        // 生成报告头部
        file << "=====================================================" << std::endl;
        file << "            DLogCover 日志覆盖率报告                " << std::endl;
        file << "=====================================================" << std::endl;
        file << "生成时间: " << getCurrentDateTimeString() << std::endl;
        file << "----------------------------------------------------" << std::endl << std::endl;

        // 进度回调
        if (progressCallback) {
            progressCallback(0.2f, "生成总体统计信息");
        }

        // 生成总体统计信息
        file << "总体覆盖率统计" << std::endl;
        file << "----------------------------------------------------" << std::endl;
        file << generateOverallStatsText(stats) << std::endl;

        // 分隔线
        file << std::endl << "=====================================================" << std::endl << std::endl;

        // 进度回调
        if (progressCallback) {
            progressCallback(0.4f, "生成文件统计信息");
        }

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
            const auto& fileStats = allStats.at(filePath);
            file << generateFileStatsText(filePath, fileStats) << std::endl;
            file << "----------------------------------------------------" << std::endl;
        }

        // 进度回调
        if (progressCallback) {
            progressCallback(0.7f, "生成未覆盖路径信息");
        }

        // 生成未覆盖路径信息
        if (!stats.uncoveredPaths.empty()) {
            file << std::endl << "未覆盖路径列表" << std::endl;
            file << "----------------------------------------------------" << std::endl;

            for (const auto& uncoveredPath : stats.uncoveredPaths) {
                file << generateUncoveredPathText(uncoveredPath) << std::endl;
            }
        }

        // 进度回调
        if (progressCallback) {
            progressCallback(0.9f, "生成改进建议");
        }

        // 生成改进建议
        file << std::endl << "改进建议" << std::endl;
        file << "----------------------------------------------------" << std::endl;
        file << generateSuggestionsText(stats) << std::endl;

        // 报告尾部
        file << std::endl << "=====================================================" << std::endl;
        file << "               报告结束                             " << std::endl;
        file << "=====================================================" << std::endl;

        file.close();

        // 完成回调
        if (progressCallback) {
            progressCallback(1.0f, "文本报告生成完成");
        }

        LOG_INFO_FMT("文本报告生成成功: %s", outputPath.c_str());
        return makeSuccess(true);
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("生成报告时发生异常: %s", e.what());
        file.close();
        return makeError<bool>(ReporterError::GENERATION_ERROR, std::string("生成报告时发生异常: ") + e.what());
    }
}

std::string TextReporterStrategy::generateOverallStatsText(const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 函数级覆盖率
    ss << "函数级覆盖率: " << std::fixed << std::setprecision(2) << stats.functionCoverage * 100 << "% ("
       << stats.coveredFunctions << "/" << stats.totalFunctions << ")" << std::endl;

    // 分支级覆盖率
    ss << "分支级覆盖率: " << std::fixed << std::setprecision(2) << stats.branchCoverage * 100 << "% ("
       << stats.coveredBranches << "/" << stats.totalBranches << ")" << std::endl;

    // 异常级覆盖率
    ss << "异常级覆盖率: " << std::fixed << std::setprecision(2) << stats.exceptionCoverage * 100 << "% ("
       << stats.coveredExceptionHandlers << "/" << stats.totalExceptionHandlers << ")" << std::endl;

    // 关键路径覆盖率
    ss << "关键路径覆盖率: " << std::fixed << std::setprecision(2) << stats.keyPathCoverage * 100 << "% ("
       << stats.coveredKeyPaths << "/" << stats.totalKeyPaths << ")" << std::endl;

    // 总体覆盖率
    ss << "总体覆盖率: " << std::fixed << std::setprecision(2) << stats.overallCoverage * 100 << "%" << std::endl;

    // 未覆盖路径总数
    ss << "未覆盖路径: " << stats.uncoveredPaths.size() << " 个" << std::endl;

    return ss.str();
}

std::string TextReporterStrategy::generateFileStatsText(const std::string& filePath,
                                                        const core::coverage::CoverageStats& stats) const {
    std::stringstream ss;

    // 显示文件路径
    ss << "文件: " << filePath << std::endl;

    // 函数覆盖率
    if (stats.totalFunctions > 0) {
        ss << "  函数覆盖率: " << std::fixed << std::setprecision(2) << (stats.functionCoverage * 100) << "% ("
           << stats.coveredFunctions << "/" << stats.totalFunctions << ")" << std::endl;
    } else {
        ss << "  函数覆盖率: N/A (0/0)" << std::endl;
    }

    // 分支覆盖率
    if (stats.totalBranches > 0) {
        ss << "  分支覆盖率: " << std::fixed << std::setprecision(2) << (stats.branchCoverage * 100) << "% ("
           << stats.coveredBranches << "/" << stats.totalBranches << ")" << std::endl;
    } else {
        ss << "  分支覆盖率: N/A (0/0)" << std::endl;
    }

    // 异常覆盖率
    if (stats.totalExceptionHandlers > 0) {
        ss << "  异常覆盖率: " << std::fixed << std::setprecision(2) << (stats.exceptionCoverage * 100) << "% ("
           << stats.coveredExceptionHandlers << "/" << stats.totalExceptionHandlers << ")" << std::endl;
    } else {
        ss << "  异常覆盖率: N/A (0/0)" << std::endl;
    }

    // 关键路径覆盖率
    if (stats.totalKeyPaths > 0) {
        ss << "  关键路径覆盖率: " << std::fixed << std::setprecision(2) << (stats.keyPathCoverage * 100) << "% ("
           << stats.coveredKeyPaths << "/" << stats.totalKeyPaths << ")" << std::endl;
    } else {
        ss << "  关键路径覆盖率: N/A (0/0)" << std::endl;
    }

    // 总体覆盖率
    ss << "  总体覆盖率: " << std::fixed << std::setprecision(2) << (stats.overallCoverage * 100) << "%";

    // 添加未覆盖路径摘要
    if (!stats.uncoveredPaths.empty()) {
        ss << std::endl << "  未覆盖路径: " << stats.uncoveredPaths.size() << " 个";
    }

    return ss.str();
}

std::string TextReporterStrategy::generateUncoveredPathText(
    const core::coverage::UncoveredPathInfo& uncoveredPath) const {
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

std::string TextReporterStrategy::generateSuggestionsText(const core::coverage::CoverageStats& stats) const {
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

std::string TextReporterStrategy::getCurrentDateTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool TextReporterStrategy::createDirectoryIfNotExists(const std::string& path) const {
    return utils::FileUtils::createDirectoryIfNotExists(path);
}

}  // namespace reporter
}  // namespace dlogcover
