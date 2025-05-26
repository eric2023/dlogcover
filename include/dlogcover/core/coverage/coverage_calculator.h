/**
 * @file coverage_calculator.h
 * @brief 覆盖率计算器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_COVERAGE_COVERAGE_CALCULATOR_H
#define DLOGCOVER_CORE_COVERAGE_COVERAGE_CALCULATOR_H

#include <dlogcover/core/coverage/coverage_stats.h>
#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/config/config.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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
     * @param astAnalyzer AST分析器引用
     * @param logIdentifier 日志识别器引用
     */
    CoverageCalculator(const config::Config& config, 
                      const ast_analyzer::ASTAnalyzer& astAnalyzer,
                      const log_identifier::LogIdentifier& logIdentifier);

    /**
     * @brief 析构函数
     */
    ~CoverageCalculator();

    /**
     * @brief 计算覆盖率
     * @return 计算结果
     */
    bool calculate();

    /**
     * @brief 获取指定文件的覆盖率统计
     * @param filePath 文件路径
     * @return 覆盖率统计
     */
    const CoverageStats& getCoverageStats(const std::string& filePath) const;

    /**
     * @brief 获取总体覆盖率统计
     * @return 总体覆盖率统计
     */
    const CoverageStats& getOverallCoverageStats() const;

    /**
     * @brief 获取所有文件的覆盖率统计
     * @return 所有文件的覆盖率统计映射表
     */
    const std::unordered_map<std::string, CoverageStats>& getAllCoverageStats() const;

private:
    const config::Config& config_;                              ///< 配置对象引用
    const ast_analyzer::ASTAnalyzer& astAnalyzer_;              ///< AST分析器引用
    const log_identifier::LogIdentifier& logIdentifier_;       ///< 日志识别器引用
    
    CoverageStats overallStats_;                                ///< 总体统计信息
    std::unordered_map<std::string, CoverageStats> coverageStats_;  ///< 文件覆盖率统计

    // 静态常量，用于识别错误处理相关的分支
    static const std::vector<std::string> ERROR_KEYWORDS;

    /**
     * @brief 计算函数覆盖率
     * @param node AST节点
     * @param filePath 文件路径
     * @param stats 覆盖率统计
     */
    void calculateFunctionCoverage(const ast_analyzer::ASTNodeInfo* node, 
                                  const std::string& filePath,
                                  CoverageStats& stats);

    /**
     * @brief 计算分支覆盖率
     * @param node AST节点
     * @param filePath 文件路径
     * @param stats 覆盖率统计
     */
    void calculateBranchCoverage(const ast_analyzer::ASTNodeInfo* node, 
                                const std::string& filePath,
                                CoverageStats& stats);

    /**
     * @brief 计算异常处理覆盖率
     * @param node AST节点
     * @param filePath 文件路径
     * @param stats 覆盖率统计
     */
    void calculateExceptionCoverage(const ast_analyzer::ASTNodeInfo* node, 
                                   const std::string& filePath,
                                   CoverageStats& stats);

    /**
     * @brief 计算关键路径覆盖率
     * @param node AST节点
     * @param filePath 文件路径
     * @param stats 覆盖率统计
     */
    void calculateKeyPathCoverage(const ast_analyzer::ASTNodeInfo* node, 
                                 const std::string& filePath,
                                 CoverageStats& stats);

    /**
     * @brief 检查节点是否包含日志宏
     * @param node AST节点
     * @return 如果包含日志宏返回true
     */
    bool containsLogMacros(const ast_analyzer::ASTNodeInfo* node) const;

    /**
     * @brief 检查子树是否有日志记录
     * @param node AST节点
     * @return 如果有日志记录返回true
     */
    bool hasLoggingInSubtree(const ast_analyzer::ASTNodeInfo* node) const;

    /**
     * @brief 识别关键分支
     * @param node AST节点
     * @param keyBranches 关键分支列表
     */
    void identifyKeyBranches(const ast_analyzer::ASTNodeInfo* node,
                            std::vector<const ast_analyzer::ASTNodeInfo*>& keyBranches);

    /**
     * @brief 合并覆盖率统计信息
     * @param overall 总体统计信息
     */
    void mergeCoverageStats(CoverageStats& overall);

    /**
     * @brief 按类型收集节点
     * @param node AST节点
     * @param types 节点类型列表
     * @param result 结果列表
     */
    void collectNodesByType(const ast_analyzer::ASTNodeInfo* node,
                           const std::vector<ast_analyzer::NodeType>& types,
                           std::vector<const ast_analyzer::ASTNodeInfo*>& result);

    /**
     * @brief 获取分支名称
     * @param node AST节点
     * @return 分支名称
     */
    std::string getBranchName(const ast_analyzer::ASTNodeInfo* node);

    /**
     * @brief 计算总体覆盖率
     * @param stats 覆盖率统计
     */
    void calculateOverallCoverage(CoverageStats& stats);

    /**
     * @brief 生成建议
     * @param type 覆盖类型
     * @param nodeType 节点类型
     * @return 建议文本
     */
    std::string generateSuggestion(CoverageType type, ast_analyzer::NodeType nodeType) const;

    /**
     * @brief 获取所有日志函数和宏
     * @return 日志函数和宏名称集合
     */
    std::unordered_set<std::string> getAllLogFunctionsAndMacros() const;
};

} // namespace coverage
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_COVERAGE_COVERAGE_CALCULATOR_H 