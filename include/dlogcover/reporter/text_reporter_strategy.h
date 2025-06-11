/**
 * @file text_reporter_strategy.h
 * @brief 文本报告生成策略
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_TEXT_REPORTER_STRATEGY_H
#define DLOGCOVER_REPORTER_TEXT_REPORTER_STRATEGY_H

#include <dlogcover/reporter/ireporter_strategy.h>
#include <dlogcover/reporter/reporter_types.h>
#include <dlogcover/core/coverage/coverage_stats.h>
#include <string>
#include <unordered_map>

namespace dlogcover {
namespace reporter {

/**
 * @brief 文本报告生成策略类
 * 
 * 实现文本格式的报告生成，提供人类可读的报告输出
 */
class TextReporterStrategy : public IReporterStrategy {
public:
    /**
     * @brief 构造函数
     */
    TextReporterStrategy() = default;

    /**
     * @brief 析构函数
     */
    ~TextReporterStrategy() override = default;

    /**
     * @brief 生成文本格式的报告
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
        return "TEXT";
    }

    /**
     * @brief 获取支持的文件扩展名
     * @return 文件扩展名
     */
    std::string getFileExtension() const override {
        return ".txt";
    }

    /**
     * @brief 获取报告格式
     * @return 报告格式
     */
    ReportFormat getFormat() const override {
        return ReportFormat::TEXT;
    }

    /**
     * @brief 设置配置
     * @param config 配置
     */
    void setConfig(const config::Config& config) override;

    /**
     * @brief 生成覆盖率条形图
     * @param percentage 百分比
     * @param width 条形图宽度
     * @return 条形图字符串
     */
    std::string generateProgressBar(double percentage, int width = 50) const;

private:
    config::Config config_; ///< 配置信息

    /**
     * @brief 生成总体统计信息的文本
     * @param stats 统计信息
     * @return 文本内容
     */
    std::string generateOverallStatsText(const core::coverage::CoverageStats& stats) const;

    /**
     * @brief 生成文件统计信息的文本
     * @param filePath 文件路径
     * @param stats 文件统计信息
     * @return 文本内容
     */
    std::string generateFileStatsText(const std::string& filePath,
                                    const core::coverage::CoverageStats& stats) const;

    /**
     * @brief 生成未覆盖路径信息的文本
     * @param uncoveredPath 未覆盖路径信息
     * @return 文本内容
     */
    std::string generateUncoveredPathText(const core::coverage::UncoveredPathInfo& uncoveredPath) const;

    /**
     * @brief 生成改进建议的文本
     * @param stats 统计信息
     * @return 文本内容
     */
    std::string generateSuggestionsText(const core::coverage::CoverageStats& stats) const;

    /**
     * @brief 获取覆盖类型的字符串表示
     * @param type 覆盖类型
     * @return 覆盖类型字符串
     */
    std::string getCoverageTypeString(const core::coverage::CoverageType& type) const;

    /**
     * @brief 获取节点类型的字符串表示
     * @param nodeType 节点类型
     * @return 节点类型字符串
     */
    std::string getNodeTypeString(const core::ast_analyzer::NodeType& nodeType) const;

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

    /**
     * @brief 格式化百分比
     * @param value 数值（0.0-1.0）
     * @return 格式化后的百分比字符串
     */
    std::string formatPercentage(double value) const;
};

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_TEXT_REPORTER_STRATEGY_H 