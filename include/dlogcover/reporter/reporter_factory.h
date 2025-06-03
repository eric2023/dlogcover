/**
 * @file reporter_factory.h
 * @brief 报告生成器工厂
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_REPORTER_FACTORY_H
#define DLOGCOVER_REPORTER_REPORTER_FACTORY_H

#include <dlogcover/reporter/ireporter_strategy.h>
#include <dlogcover/reporter/reporter_types.h>
#include <dlogcover/reporter/text_reporter_strategy.h>
#include <dlogcover/reporter/json_reporter_strategy.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dlogcover {
namespace reporter {

/**
 * @brief 报告生成器工厂类
 * 
 * 使用工厂模式创建不同类型的报告生成策略
 * 支持注册新的策略类型和获取已有策略
 */
class ReporterFactory {
public:
    /**
     * @brief 获取工厂单例实例
     * @return 工厂实例引用
     */
    static ReporterFactory& getInstance();

    /**
     * @brief 创建指定格式的报告策略
     * @param format 报告格式
     * @return 报告策略智能指针
     */
    std::shared_ptr<IReporterStrategy> createStrategy(ReportFormat format);

    /**
     * @brief 创建指定格式的报告策略
     * @param formatStr 报告格式字符串
     * @return 报告策略智能指针
     */
    std::shared_ptr<IReporterStrategy> createStrategy(const std::string& formatStr);

    /**
     * @brief 注册报告策略
     * @param format 报告格式
     * @param strategy 策略实例
     * @return 注册成功返回true
     */
    bool registerStrategy(ReportFormat format, std::shared_ptr<IReporterStrategy> strategy);

    /**
     * @brief 获取支持的报告格式列表
     * @return 支持的格式列表
     */
    std::vector<ReportFormat> getSupportedFormats() const;

    /**
     * @brief 检查是否支持指定格式
     * @param format 报告格式
     * @return 支持返回true
     */
    bool isFormatSupported(ReportFormat format) const;

    /**
     * @brief 获取默认格式
     * @return 默认报告格式
     */
    ReportFormat getDefaultFormat() const { return ReportFormat::TEXT; }

    // 禁用拷贝构造和赋值
    ReporterFactory(const ReporterFactory&) = delete;
    ReporterFactory& operator=(const ReporterFactory&) = delete;

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    ReporterFactory();

    /**
     * @brief 析构函数
     */
    ~ReporterFactory() = default;

    /**
     * @brief 初始化内置策略
     */
    void init();

private:
    /// 策略映射表
    std::unordered_map<ReportFormat, std::shared_ptr<IReporterStrategy>> strategies_;
};

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_REPORTER_FACTORY_H 