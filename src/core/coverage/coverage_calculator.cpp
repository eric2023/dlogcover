/**
 * @file coverage_calculator.cpp
 * @brief 覆盖率计算器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace coverage {

CoverageCalculator::CoverageCalculator(const config::Config& config, const ast_analyzer::ASTAnalyzer& astAnalyzer,
                                       const log_identifier::LogIdentifier& logIdentifier)
    : config_(config), astAnalyzer_(astAnalyzer), logIdentifier_(logIdentifier) {
    LOG_DEBUG("覆盖率计算器初始化");
}

CoverageCalculator::~CoverageCalculator() {
    LOG_DEBUG("覆盖率计算器销毁");
}

bool CoverageCalculator::calculate() {
    LOG_INFO("开始计算覆盖率");

    coverageStats_.clear();

    // 获取所有AST节点信息
    const auto& astNodes = astAnalyzer_.getAllASTNodeInfo();

    // 遍历所有文件的AST节点
    for (const auto& [filePath, nodeInfo] : astNodes) {
        LOG_INFO_FMT("计算文件的覆盖率: %s", filePath.c_str());

        // 初始化该文件的覆盖率统计信息
        CoverageStats stats;

        // 计算各种类型的覆盖率
        if (config_.analysis.functionCoverage) {
            calculateFunctionCoverage(nodeInfo.get(), filePath, stats);
        }

        if (config_.analysis.branchCoverage) {
            calculateBranchCoverage(nodeInfo.get(), filePath, stats);
        }

        if (config_.analysis.exceptionCoverage) {
            calculateExceptionCoverage(nodeInfo.get(), filePath, stats);
        }

        if (config_.analysis.keyPathCoverage) {
            calculateKeyPathCoverage(nodeInfo.get(), filePath, stats);
        }

        // 计算总体覆盖率
        stats.overallCoverage = 0.0;
        int totalFactors = 0;

        if (config_.analysis.functionCoverage) {
            stats.overallCoverage += stats.functionCoverage;
            totalFactors++;
        }

        if (config_.analysis.branchCoverage) {
            stats.overallCoverage += stats.branchCoverage;
            totalFactors++;
        }

        if (config_.analysis.exceptionCoverage) {
            stats.overallCoverage += stats.exceptionCoverage;
            totalFactors++;
        }

        if (config_.analysis.keyPathCoverage) {
            stats.overallCoverage += stats.keyPathCoverage;
            totalFactors++;
        }

        if (totalFactors > 0) {
            stats.overallCoverage /= totalFactors;
        }

        // 保存该文件的覆盖率统计信息
        coverageStats_[filePath] = stats;
    }

    // 计算总体覆盖率
    mergeCoverageStats(overallStats_);

    LOG_INFO("覆盖率计算完成");
    return true;
}

const CoverageStats& CoverageCalculator::getCoverageStats(const std::string& filePath) const {
    static const CoverageStats emptyCoverageStats;

    auto it = coverageStats_.find(filePath);
    if (it != coverageStats_.end()) {
        return it->second;
    }

    return emptyCoverageStats;
}

const CoverageStats& CoverageCalculator::getOverallCoverageStats() const {
    return overallStats_;
}

const std::unordered_map<std::string, CoverageStats>& CoverageCalculator::getAllCoverageStats() const {
    return coverageStats_;
}

void CoverageCalculator::calculateFunctionCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                   CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 这是一个简单的实现框架，实际情况需要更复杂的分析

    // 计数器
    int totalFunctions = 0;
    int coveredFunctions = 0;

    // 检查节点类型是否为函数或方法
    if (node->type == ast_analyzer::NodeType::FUNCTION || node->type == ast_analyzer::NodeType::METHOD) {
        totalFunctions++;

        // 检查函数是否包含日志调用
        if (node->hasLogging) {
            coveredFunctions++;
        } else {
            // 添加到未覆盖路径
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::FUNCTION;
            uncoveredPath.nodeType = node->type;
            uncoveredPath.location = node->location;
            uncoveredPath.name = node->name;
            uncoveredPath.text = node->text;
            uncoveredPath.suggestion = generateSuggestion(CoverageType::FUNCTION, node->type);

            stats.uncoveredPaths.push_back(uncoveredPath);
        }
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        calculateFunctionCoverage(child.get(), filePath, stats);
    }

    // 更新覆盖率统计信息
    stats.totalFunctions += totalFunctions;
    stats.coveredFunctions += coveredFunctions;

    if (stats.totalFunctions > 0) {
        stats.functionCoverage = static_cast<double>(stats.coveredFunctions) / stats.totalFunctions;
    } else {
        stats.functionCoverage = 1.0;  // 没有函数视为100%覆盖
    }
}

void CoverageCalculator::calculateBranchCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                 CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 这是一个简单的实现框架，实际情况需要更复杂的分析

    // 计数器
    int totalBranches = 0;
    int coveredBranches = 0;

    // 检查节点类型是否为分支语句
    if (node->type == ast_analyzer::NodeType::IF_STMT || node->type == ast_analyzer::NodeType::ELSE_STMT ||
        node->type == ast_analyzer::NodeType::SWITCH_STMT || node->type == ast_analyzer::NodeType::CASE_STMT) {
        totalBranches++;

        // 检查分支是否包含日志调用
        if (node->hasLogging) {
            coveredBranches++;
        } else {
            // 添加到未覆盖路径
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::BRANCH;
            uncoveredPath.nodeType = node->type;
            uncoveredPath.location = node->location;
            uncoveredPath.name = node->name;
            uncoveredPath.text = node->text;
            uncoveredPath.suggestion = generateSuggestion(CoverageType::BRANCH, node->type);

            stats.uncoveredPaths.push_back(uncoveredPath);
        }
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        calculateBranchCoverage(child.get(), filePath, stats);
    }

    // 更新覆盖率统计信息
    stats.totalBranches += totalBranches;
    stats.coveredBranches += coveredBranches;

    if (stats.totalBranches > 0) {
        stats.branchCoverage = static_cast<double>(stats.coveredBranches) / stats.totalBranches;
    } else {
        stats.branchCoverage = 1.0;  // 没有分支视为100%覆盖
    }
}

void CoverageCalculator::calculateExceptionCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                    CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 这是一个简单的实现框架，实际情况需要更复杂的分析

    // 计数器
    int totalExceptionHandlers = 0;
    int coveredExceptionHandlers = 0;

    // 检查节点类型是否为异常处理相关
    if (node->type == ast_analyzer::NodeType::TRY_STMT || node->type == ast_analyzer::NodeType::CATCH_STMT) {
        totalExceptionHandlers++;

        // 检查异常处理是否包含日志调用
        if (node->hasLogging) {
            coveredExceptionHandlers++;
        } else {
            // 添加到未覆盖路径
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::EXCEPTION;
            uncoveredPath.nodeType = node->type;
            uncoveredPath.location = node->location;
            uncoveredPath.name = node->name;
            uncoveredPath.text = node->text;
            uncoveredPath.suggestion = generateSuggestion(CoverageType::EXCEPTION, node->type);

            stats.uncoveredPaths.push_back(uncoveredPath);
        }
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        calculateExceptionCoverage(child.get(), filePath, stats);
    }

    // 更新覆盖率统计信息
    stats.totalExceptionHandlers += totalExceptionHandlers;
    stats.coveredExceptionHandlers += coveredExceptionHandlers;

    if (stats.totalExceptionHandlers > 0) {
        stats.exceptionCoverage = static_cast<double>(stats.coveredExceptionHandlers) / stats.totalExceptionHandlers;
    } else {
        stats.exceptionCoverage = 1.0;  // 没有异常处理视为100%覆盖
    }
}

void CoverageCalculator::calculateKeyPathCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                  CoverageStats& stats) {
    // 关键路径覆盖率计算比较复杂，这里提供一个简单的框架

    // 在实际实现中，需要识别程序的关键执行路径，然后统计哪些路径上有日志调用

    // 这里我们简单地设置一个值
    stats.keyPathCoverage = 0.8;  // 示例值
}

void CoverageCalculator::mergeCoverageStats(CoverageStats& overall) {
    // 重置总体统计
    overall = CoverageStats();

    // 如果没有文件，直接返回
    if (coverageStats_.empty()) {
        return;
    }

    // 累计各项指标
    for (const auto& [filePath, stats] : coverageStats_) {
        overall.totalFunctions += stats.totalFunctions;
        overall.coveredFunctions += stats.coveredFunctions;

        overall.totalBranches += stats.totalBranches;
        overall.coveredBranches += stats.coveredBranches;

        overall.totalExceptionHandlers += stats.totalExceptionHandlers;
        overall.coveredExceptionHandlers += stats.coveredExceptionHandlers;

        overall.totalKeyPaths += stats.totalKeyPaths;
        overall.coveredKeyPaths += stats.coveredKeyPaths;

        // 累计未覆盖路径
        overall.uncoveredPaths.insert(overall.uncoveredPaths.end(), stats.uncoveredPaths.begin(),
                                      stats.uncoveredPaths.end());
    }

    // 计算各项覆盖率
    if (overall.totalFunctions > 0) {
        overall.functionCoverage = static_cast<double>(overall.coveredFunctions) / overall.totalFunctions;
    } else {
        overall.functionCoverage = 1.0;
    }

    if (overall.totalBranches > 0) {
        overall.branchCoverage = static_cast<double>(overall.coveredBranches) / overall.totalBranches;
    } else {
        overall.branchCoverage = 1.0;
    }

    if (overall.totalExceptionHandlers > 0) {
        overall.exceptionCoverage =
            static_cast<double>(overall.coveredExceptionHandlers) / overall.totalExceptionHandlers;
    } else {
        overall.exceptionCoverage = 1.0;
    }

    if (overall.totalKeyPaths > 0) {
        overall.keyPathCoverage = static_cast<double>(overall.coveredKeyPaths) / overall.totalKeyPaths;
    } else {
        overall.keyPathCoverage = 1.0;
    }

    // 计算总体覆盖率
    int totalFactors = 0;
    overall.overallCoverage = 0.0;

    if (config_.analysis.functionCoverage) {
        overall.overallCoverage += overall.functionCoverage;
        totalFactors++;
    }

    if (config_.analysis.branchCoverage) {
        overall.overallCoverage += overall.branchCoverage;
        totalFactors++;
    }

    if (config_.analysis.exceptionCoverage) {
        overall.overallCoverage += overall.exceptionCoverage;
        totalFactors++;
    }

    if (config_.analysis.keyPathCoverage) {
        overall.overallCoverage += overall.keyPathCoverage;
        totalFactors++;
    }

    if (totalFactors > 0) {
        overall.overallCoverage /= totalFactors;
    }
}

std::string CoverageCalculator::generateSuggestion(CoverageType type, ast_analyzer::NodeType nodeType) const {
    // 根据覆盖率类型和节点类型生成建议
    std::string suggestion;

    switch (type) {
        case CoverageType::FUNCTION:
            if (nodeType == ast_analyzer::NodeType::FUNCTION || nodeType == ast_analyzer::NodeType::METHOD) {
                suggestion = "建议在函数入口和重要返回点添加日志";
            }
            break;

        case CoverageType::BRANCH:
            if (nodeType == ast_analyzer::NodeType::IF_STMT) {
                suggestion = "建议在if条件成立时添加日志";
            } else if (nodeType == ast_analyzer::NodeType::ELSE_STMT) {
                suggestion = "建议在else分支添加日志";
            } else if (nodeType == ast_analyzer::NodeType::SWITCH_STMT ||
                       nodeType == ast_analyzer::NodeType::CASE_STMT) {
                suggestion = "建议在switch/case分支添加日志";
            }
            break;

        case CoverageType::EXCEPTION:
            if (nodeType == ast_analyzer::NodeType::TRY_STMT) {
                suggestion = "建议在try块开始添加调试日志";
            } else if (nodeType == ast_analyzer::NodeType::CATCH_STMT) {
                suggestion = "建议在catch块添加错误/警告日志";
            }
            break;

        case CoverageType::KEY_PATH:
            suggestion = "建议在关键执行路径添加日志";
            break;

        default:
            suggestion = "建议添加适当的日志";
    }

    return suggestion;
}

}  // namespace coverage
}  // namespace core
}  // namespace dlogcover
