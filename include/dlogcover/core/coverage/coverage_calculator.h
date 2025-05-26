/**
 * @file coverage_calculator.h
 * @brief 覆盖率计算器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_COVERAGE_COVERAGE_CALCULATOR_H
#define DLOGCOVER_CORE_COVERAGE_COVERAGE_CALCULATOR_H

#include <dlogcover/core/coverage/coverage_stats.h>
#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/config/config.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace dlogcover {
namespace core {
namespace coverage {

/**
 * @brief 覆盖率计算器类
 * 
 * 负责计算各种类型的代码覆盖率
 */
class CoverageCalculator {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     */
    explicit CoverageCalculator(const config::Config& config);

    /**
     * @brief 析构函数
     */
    ~CoverageCalculator();

    /**
     * @brief 计算指定文件的覆盖率
     * @param filePath 文件路径
     * @param astNodeInfo AST节点信息
     * @return 计算结果
     */
    ast_analyzer::Result<FileCoverageStats> calculateFileCoverage(
        const std::string& filePath, 
        const ast_analyzer::ASTNodeInfo& astNodeInfo);

    /**
     * @brief 计算所有文件的覆盖率
     * @param astNodes 所有文件的AST节点信息
     * @return 计算结果
     */
    ast_analyzer::Result<bool> calculateAllCoverage(
        const std::unordered_map<std::string, std::unique_ptr<ast_analyzer::ASTNodeInfo>>& astNodes);

    /**
     * @brief 获取总体覆盖率统计
     * @return 总体覆盖率统计
     */
    const CoverageStats& getOverallCoverageStats() const;

    /**
     * @brief 获取所有文件的覆盖率统计
     * @return 所有文件的覆盖率统计列表
     */
    const std::vector<FileCoverageStats>& getAllCoverageStats() const;

    /**
     * @brief 获取指定文件的覆盖率统计
     * @param filePath 文件路径
     * @return 文件覆盖率统计指针，未找到返回nullptr
     */
    const FileCoverageStats* getFileCoverageStats(const std::string& filePath) const;

private:
    const config::Config& config_;                              ///< 配置对象引用
    CoverageStats overallStats_;                                ///< 总体统计信息
    std::vector<FileCoverageStats> allFileStats_;               ///< 所有文件统计信息
    std::unordered_map<std::string, size_t> fileStatsIndex_;    ///< 文件统计索引

    /**
     * @brief 计算函数覆盖率
     * @param astNodeInfo AST节点信息
     * @param fileStats 文件统计信息
     */
    void calculateFunctionCoverage(const ast_analyzer::ASTNodeInfo& astNodeInfo, FileCoverageStats& fileStats);

    /**
     * @brief 计算分支覆盖率
     * @param astNodeInfo AST节点信息
     * @param fileStats 文件统计信息
     */
    void calculateBranchCoverage(const ast_analyzer::ASTNodeInfo& astNodeInfo, FileCoverageStats& fileStats);

    /**
     * @brief 计算异常处理覆盖率
     * @param astNodeInfo AST节点信息
     * @param fileStats 文件统计信息
     */
    void calculateExceptionCoverage(const ast_analyzer::ASTNodeInfo& astNodeInfo, FileCoverageStats& fileStats);

    /**
     * @brief 计算关键路径覆盖率
     * @param astNodeInfo AST节点信息
     * @param fileStats 文件统计信息
     */
    void calculateKeyPathCoverage(const ast_analyzer::ASTNodeInfo& astNodeInfo, FileCoverageStats& fileStats);

    /**
     * @brief 更新总体统计信息
     */
    void updateOverallStats();

    /**
     * @brief 计算覆盖率比例
     * @param covered 已覆盖数量
     * @param total 总数量
     * @return 覆盖率比例
     */
    double calculateRatio(int covered, int total) const;
};

} // namespace coverage
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_COVERAGE_COVERAGE_CALCULATOR_H 