/**
 * @file reporter.cpp
 * @brief 报告生成器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/reporter/reporter.h>
#include <dlogcover/reporter/reporter_factory.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace dlogcover {
namespace reporter {

Reporter::Reporter(const config::Config& config, const core::coverage::CoverageCalculator& coverageCalculator)
    : config_(config), coverageCalculator_(coverageCalculator) {
    LOG_DEBUG("报告生成器初始化");

    // 根据配置创建默认策略
    // 注释掉config.report.format，因为新配置结构中没有这个字段，使用默认格式
    // ReportFormat format = parseReportFormat(config.report.format);
    ReportFormat format = ReportFormat::TEXT; // 默认使用文本格式
    strategy_ = ReporterFactory::getInstance().createStrategy(format);
    strategy_->setConfig(config_);
    LOG_DEBUG_FMT("使用默认报告策略: %s", strategy_->getName().c_str());
}

Reporter::~Reporter() {
    LOG_DEBUG("报告生成器销毁");
}

Result<bool> Reporter::generateReport(const std::string& outputPath, const ProgressCallback& progressCallback) {
    LOG_INFO_FMT("生成报告: %s", outputPath.c_str());

    // 验证路径
    auto validateResult = validateAndCreatePath(outputPath);
    if (validateResult.hasError()) {
        return validateResult;
    }

    // 获取统计数据
    const auto& overallStats = coverageCalculator_.getOverallCoverageStats();
    const auto& allStats = coverageCalculator_.getAllCoverageStats();

    // 使用当前策略生成报告
    auto result = strategy_->generateReport(outputPath, overallStats, allStats, progressCallback);

    if (result.hasError()) {
        LOG_ERROR_FMT("报告生成失败: %s, 错误: %s", outputPath.c_str(), result.errorMessage().c_str());
    } else {
        LOG_INFO_FMT("报告生成成功: %s", outputPath.c_str());
    }

    return result;
}

Result<bool> Reporter::generateReport(const std::string& outputPath, ReportFormat format,
                                      const ProgressCallback& progressCallback) {
    LOG_INFO_FMT("生成%s格式报告: %s", getReportFormatString(format).c_str(), outputPath.c_str());

    // 获取指定格式的策略
    auto tmpStrategy = ReporterFactory::getInstance().createStrategy(format);
    tmpStrategy->setConfig(config_);

    // 临时设置策略
    auto oldStrategy = strategy_;
    strategy_ = tmpStrategy;

    // 生成报告
    auto result = generateReport(outputPath, progressCallback);

    // 恢复原策略
    strategy_ = oldStrategy;

    return result;
}

void Reporter::setStrategy(std::shared_ptr<IReporterStrategy> strategy) {
    if (!strategy) {
        LOG_WARNING("尝试设置空策略，操作被忽略");
        return;
    }

    LOG_DEBUG_FMT("设置报告策略: %s", strategy->getName().c_str());
    strategy->setConfig(config_);
    strategy_ = strategy;
}

std::shared_ptr<IReporterStrategy> Reporter::getStrategy() const {
    return strategy_;
}

std::vector<ReportFormat> Reporter::getSupportedFormats() const {
    return ReporterFactory::getInstance().getSupportedFormats();
}

Result<bool> Reporter::validateAndCreatePath(const std::string& path) const {
    // 如果路径为空
    if (path.empty()) {
        LOG_ERROR("输出路径为空");
        return makeError<bool>(ReporterError::INVALID_PATH, "输出路径不能为空");
    }

    try {
        // 获取目录部分
        std::filesystem::path filePath(path);
        std::filesystem::path dirPath = filePath.parent_path();

        // 如果目录不存在，尝试创建
        if (!dirPath.empty() && !std::filesystem::exists(dirPath)) {
            LOG_DEBUG_FMT("创建目录: %s", dirPath.string().c_str());

            if (!utils::FileUtils::createDirectoryIfNotExists(dirPath.string())) {
                LOG_ERROR_FMT("无法创建目录: %s", dirPath.string().c_str());
                return makeError<bool>(ReporterError::DIRECTORY_ERROR, "无法创建目录: " + dirPath.string());
            }
        }

        return makeSuccess(true);
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("路径验证异常: %s", e.what());
        return makeError<bool>(ReporterError::INVALID_PATH, std::string("路径验证异常: ") + e.what());
    }
}

// 旧的generateTextReport方法已移除，改为使用TextReporterStrategy实现

// 旧的generateJsonReport方法已移除，改为使用JsonReporterStrategy实现

}  // namespace reporter
}  // namespace dlogcover
