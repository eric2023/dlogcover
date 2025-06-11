/**
 * @file json_reporter_strategy.h
 * @brief JSON报告生成策略
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_JSON_REPORTER_STRATEGY_H
#define DLOGCOVER_REPORTER_JSON_REPORTER_STRATEGY_H

#include <dlogcover/reporter/ireporter_strategy.h>
#include <dlogcover/reporter/reporter_types.h>
#include <dlogcover/core/coverage/coverage_stats.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace dlogcover {
namespace reporter {

/**
 * @brief JSON报告生成策略类
 * 
 * 实现JSON格式的报告生成，支持结构化的数据输出
 */
class JsonReporterStrategy : public IReporterStrategy {
public:
    /**
     * @brief 构造函数
     */
    JsonReporterStrategy() = default;

    /**
     * @brief 析构函数
     */
    ~JsonReporterStrategy() override = default;

    /**
     * @brief 生成JSON格式的报告
     * @param outputPath 输出文件路径
     * @param stats 总体统计信息
     * @param allStats 所有文件的统计信息映射
     * @param progressCallback 进度回调函数
     * @return 生成结果
     */
    Result<bool> generateReport(
        const std::string& outputPath,
        const core::coverage::CoverageStats& stats,
        const std::unordered_map<std::string, core::coverage::CoverageStats>& allStats,
        const ProgressCallback& progressCallback = nullptr) override;

    /**
     * @brief 获取策略名称
     * @return 策略名称
     */
    std::string getName() const override {
        return "JSON";
    }

    /**
     * @brief 获取支持的文件扩展名
     * @return 文件扩展名
     */
    std::string getFileExtension() const override {
        return ".json";
    }

    /**
     * @brief 获取报告格式
     * @return 报告格式
     */
    ReportFormat getFormat() const override {
        return ReportFormat::JSON;
    }

    /**
     * @brief 设置配置
     * @param config 配置
     */
    void setConfig(const config::Config& config) override;

private:
    config::Config config_; ///< 配置信息

    /**
     * @brief 创建统计信息的JSON对象
     * @param stats 统计信息
     * @return JSON对象
     */
    nlohmann::json createStatsJson(const core::coverage::CoverageStats& stats) const;

    /**
     * @brief 创建文件统计信息的JSON对象
     * @param filePath 文件路径
     * @param stats 文件统计信息
     * @return JSON对象
     */
    nlohmann::json createFileStatsJson(const std::string& filePath,
                                     const core::coverage::CoverageStats& stats) const;

    /**
     * @brief 创建未覆盖路径信息的JSON对象
     * @param uncoveredPath 未覆盖路径信息
     * @return JSON对象
     */
    nlohmann::json createUncoveredPathJson(const core::coverage::UncoveredPathInfo& uncoveredPath) const;

    /**
     * @brief 创建改进建议的JSON对象
     * @param stats 统计信息
     * @return JSON对象
     */
    nlohmann::json createSuggestionsJson(const core::coverage::CoverageStats& stats) const;

    /**
     * @brief 获取当前日期时间字符串
     * @return 日期时间字符串
     */
    std::string getCurrentDateTimeString() const;

    /**
     * @brief 检查并创建目录
     * @param dirPath 目录路径
     * @return 成功返回true
     */
    bool createDirectoryIfNotExists(const std::string& dirPath) const;
};

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_JSON_REPORTER_STRATEGY_H 