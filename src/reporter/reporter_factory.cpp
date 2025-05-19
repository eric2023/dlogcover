/**
 * @file reporter_factory.cpp
 * @brief 报告生成器工厂实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/reporter/reporter_factory.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace reporter {

ReporterFactory& ReporterFactory::getInstance() {
    static ReporterFactory instance;
    return instance;
}

ReporterFactory::ReporterFactory() {
    LOG_DEBUG("初始化报告生成器工厂");
    init();
}

void ReporterFactory::init() {
    // 注册内置策略
    strategies_[ReportFormat::TEXT] = std::make_shared<TextReporterStrategy>();
    strategies_[ReportFormat::JSON] = std::make_shared<JsonReporterStrategy>();

    LOG_DEBUG_FMT("已注册 %zu 种报告格式策略", strategies_.size());
}

std::shared_ptr<IReporterStrategy> ReporterFactory::createStrategy(ReportFormat format) {
    LOG_DEBUG_FMT("创建报告策略: %s", getReportFormatString(format).c_str());

    auto it = strategies_.find(format);
    if (it != strategies_.end()) {
        return it->second;
    }

    LOG_WARNING_FMT("未找到报告格式 %s 的策略，使用默认文本格式", getReportFormatString(format).c_str());
    return strategies_[ReportFormat::TEXT];
}

std::shared_ptr<IReporterStrategy> ReporterFactory::createStrategy(const std::string& formatStr) {
    return createStrategy(parseReportFormat(formatStr));
}

bool ReporterFactory::registerStrategy(ReportFormat format, std::shared_ptr<IReporterStrategy> strategy) {
    if (!strategy) {
        LOG_ERROR("无法注册空策略");
        return false;
    }

    LOG_INFO_FMT("注册报告格式策略: %s", getReportFormatString(format).c_str());
    strategies_[format] = strategy;
    return true;
}

std::vector<ReportFormat> ReporterFactory::getSupportedFormats() const {
    std::vector<ReportFormat> formats;
    for (const auto& [format, _] : strategies_) {
        formats.push_back(format);
    }
    return formats;
}

}  // namespace reporter
}  // namespace dlogcover
