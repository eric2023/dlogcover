/**
 * @file ireporter_strategy.h
 * @brief 报告策略接口
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_IREPORTER_STRATEGY_H
#define DLOGCOVER_REPORTER_IREPORTER_STRATEGY_H

#include <dlogcover/reporter/reporter_types.h>
#include <dlogcover/core/coverage/coverage_stats.h>
#include <functional>
#include <string>
#include <vector>

namespace dlogcover {
namespace reporter {

/**
 * @brief 进度回调函数类型
 */
using ProgressCallback = std::function<void(int current, int total, const std::string& message)>;

/**
 * @brief 报告策略接口
 * 
 * 定义了生成不同格式报告的通用接口
 */
class IReporterStrategy {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IReporterStrategy() = default;

    /**
     * @brief 生成报告
     * @param outputPath 输出路径
     * @param overallStats 总体统计信息
     * @param allStats 所有文件的统计信息
     * @param progressCallback 进度回调函数
     * @return 生成结果
     */
    virtual Result<bool> generateReport(
        const std::string& outputPath,
        const core::coverage::CoverageStats& overallStats,
        const std::vector<core::coverage::CoverageStats>& allStats,
        const ProgressCallback& progressCallback = nullptr) = 0;

    /**
     * @brief 获取策略名称
     * @return 策略名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取支持的文件扩展名
     * @return 文件扩展名
     */
    virtual std::string getFileExtension() const = 0;

    /**
     * @brief 获取报告格式
     * @return 报告格式
     */
    virtual ReportFormat getFormat() const = 0;
};

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_IREPORTER_STRATEGY_H 