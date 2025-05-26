/**
 * @file reporter.h
 * @brief 报告生成器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_REPORTER_H
#define DLOGCOVER_REPORTER_REPORTER_H

#include <dlogcover/config/config.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/reporter/reporter_types.h>
#include <dlogcover/reporter/ireporter_strategy.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dlogcover {
namespace reporter {

/**
 * @brief 报告生成器类
 * 
 * 负责生成各种格式的覆盖率报告
 */
class Reporter {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     * @param coverageCalculator 覆盖率计算器引用
     */
    Reporter(const config::Config& config, const core::coverage::CoverageCalculator& coverageCalculator);

    /**
     * @brief 析构函数
     */
    ~Reporter();

    /**
     * @brief 生成报告（使用默认格式）
     * @param outputPath 输出路径
     * @param progressCallback 进度回调函数
     * @return 生成结果，成功返回true，否则返回错误信息
     */
    Result<bool> generateReport(const std::string& outputPath, 
                               const ProgressCallback& progressCallback = nullptr);

    /**
     * @brief 生成指定格式的报告
     * @param outputPath 输出路径
     * @param format 报告格式
     * @param progressCallback 进度回调函数
     * @return 生成结果，成功返回true，否则返回错误信息
     */
    Result<bool> generateReport(const std::string& outputPath, 
                               ReportFormat format,
                               const ProgressCallback& progressCallback = nullptr);

    /**
     * @brief 设置报告策略
     * @param strategy 报告策略
     */
    void setStrategy(std::shared_ptr<IReporterStrategy> strategy);

    /**
     * @brief 获取当前报告策略
     * @return 当前报告策略
     */
    std::shared_ptr<IReporterStrategy> getStrategy() const;

    /**
     * @brief 获取支持的报告格式
     * @return 支持的报告格式列表
     */
    std::vector<ReportFormat> getSupportedFormats() const;

private:
    const config::Config& config_;                                      ///< 配置对象引用
    const core::coverage::CoverageCalculator& coverageCalculator_;      ///< 覆盖率计算器引用
    std::shared_ptr<IReporterStrategy> strategy_;                       ///< 当前报告策略

    /**
     * @brief 验证并创建输出路径
     * @param path 输出路径
     * @return 验证结果
     */
    Result<bool> validateAndCreatePath(const std::string& path) const;
};

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_REPORTER_H 