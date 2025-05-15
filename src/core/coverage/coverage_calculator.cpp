/**
 * @file coverage_calculator.cpp
 * @brief 覆盖率计算器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>

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
    overallStats_ = CoverageStats();

    // 获取所有AST节点信息
    const auto& astNodes = astAnalyzer_.getAllASTNodeInfo();

    // 确保所有文件都有覆盖率统计信息
    for (const auto& [filePath, _] : astNodes) {
        coverageStats_[filePath] = CoverageStats();
    }

    // 计算每个文件的覆盖率
    for (const auto& [filePath, nodeInfo] : astNodes) {
        LOG_INFO_FMT("计算文件覆盖率: %s", filePath.c_str());

        // 获取文件的覆盖率统计信息
        auto& stats = coverageStats_[filePath];

        // 检查分析配置，计算相应类型的覆盖率
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
        calculateOverallCoverage(stats);

        LOG_INFO_FMT("文件 %s 的覆盖率计算完成: %.2f%%", filePath.c_str(), stats.overallCoverage * 100);
    }

    // 合并所有文件的覆盖率统计信息
    mergeCoverageStats(overallStats_);

    // 输出总体覆盖率信息
    LOG_INFO_FMT("总体函数覆盖率: %.2f%% (%d/%d)", overallStats_.functionCoverage * 100, overallStats_.coveredFunctions,
                 overallStats_.totalFunctions);

    LOG_INFO_FMT("总体分支覆盖率: %.2f%% (%d/%d)", overallStats_.branchCoverage * 100, overallStats_.coveredBranches,
                 overallStats_.totalBranches);

    LOG_INFO_FMT("总体异常覆盖率: %.2f%% (%d/%d)", overallStats_.exceptionCoverage * 100,
                 overallStats_.coveredExceptionHandlers, overallStats_.totalExceptionHandlers);

    LOG_INFO_FMT("总体关键路径覆盖率: %.2f%% (%d/%d)", overallStats_.keyPathCoverage * 100,
                 overallStats_.coveredKeyPaths, overallStats_.totalKeyPaths);

    LOG_INFO_FMT("总体覆盖率: %.2f%%", overallStats_.overallCoverage * 100);

    LOG_INFO_FMT("发现未覆盖路径: %zu 个", overallStats_.uncoveredPaths.size());

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

    // 遍历AST节点树，收集函数节点
    std::vector<const ast_analyzer::ASTNodeInfo*> functionNodes;
    collectNodesByType(node, {ast_analyzer::NodeType::FUNCTION, ast_analyzer::NodeType::METHOD}, functionNodes);

    LOG_DEBUG_FMT("文件 %s 中发现 %zu 个函数/方法", filePath.c_str(), functionNodes.size());

    // 更新函数总数
    stats.totalFunctions += functionNodes.size();

    // 获取该文件的日志调用
    const auto& logCalls = logIdentifier_.getLogCalls(filePath);

    // 创建日志行号集合，用于快速查找
    std::unordered_set<unsigned int> logLines;
    for (const auto& logCall : logCalls) {
        logLines.insert(logCall.location.line);
    }

    // 计算已覆盖的函数数量
    for (const auto* funcNode : functionNodes) {
        bool isCovered = funcNode->hasLogging;

        // 如果节点本身没有标记为有日志，检查它的子节点
        if (!isCovered) {
            for (const auto& child : funcNode->children) {
                if (child->hasLogging) {
                    isCovered = true;
                    break;
                }
            }
        }

        // 如果仍然未覆盖，检查是否在logCalls中有记录
        if (!isCovered) {
            for (const auto& logCall : logCalls) {
                // 检查日志调用是否在函数的范围内
                // 这是一个简化的检查，只比较行号
                if (logCall.location.line >= funcNode->location.line &&
                    logCall.location.line <= funcNode->location.line + 10) { // 假设函数不超过10行
                    isCovered = true;
                    break;
                }
            }
        }

        if (!isCovered) {
            // 创建未覆盖路径信息
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::FUNCTION;
            uncoveredPath.nodeType = funcNode->type;
            uncoveredPath.location = funcNode->location;
            uncoveredPath.name = funcNode->name;
            uncoveredPath.text = funcNode->text.substr(0, 100) + (funcNode->text.length() > 100 ? "..." : "");
            uncoveredPath.suggestion = generateSuggestion(CoverageType::FUNCTION, funcNode->type);

            // 添加到未覆盖路径列表
            stats.uncoveredPaths.push_back(uncoveredPath);
        } else {
            stats.coveredFunctions++;
        }
    }

    // 计算函数覆盖率
    if (stats.totalFunctions > 0) {
        stats.functionCoverage = static_cast<double>(stats.coveredFunctions) / stats.totalFunctions;
    }

    LOG_DEBUG_FMT("函数覆盖率: %.2f%% (%d/%d)", stats.functionCoverage * 100, stats.coveredFunctions,
                  stats.totalFunctions);
}

void CoverageCalculator::calculateBranchCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                 CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 遍历AST节点树，收集分支节点
    std::vector<const ast_analyzer::ASTNodeInfo*> branchNodes;
    collectNodesByType(
        node,
        {ast_analyzer::NodeType::IF_STMT, ast_analyzer::NodeType::ELSE_STMT, ast_analyzer::NodeType::SWITCH_STMT,
         ast_analyzer::NodeType::CASE_STMT, ast_analyzer::NodeType::FOR_STMT, ast_analyzer::NodeType::WHILE_STMT,
         ast_analyzer::NodeType::DO_STMT},
        branchNodes);

    LOG_DEBUG_FMT("文件 %s 中发现 %zu 个分支", filePath.c_str(), branchNodes.size());

    // 更新分支总数
    stats.totalBranches += branchNodes.size();

    // 获取该文件的日志调用
    const auto& logCalls = logIdentifier_.getLogCalls(filePath);

    // 创建日志行号区间集合，用于判断分支是否覆盖
    std::vector<std::pair<unsigned int, unsigned int>> logLineRanges;
    for (const auto& logCall : logCalls) {
        // 为每个日志调用创建一个小范围的区间，±2行
        unsigned int startLine = (logCall.location.line > 2) ? (logCall.location.line - 2) : 1;
        unsigned int endLine = logCall.location.line + 2;
        logLineRanges.push_back({startLine, endLine});
    }

    // 计算已覆盖的分支数量
    for (const auto* branchNode : branchNodes) {
        bool isCovered = branchNode->hasLogging;

        // 如果节点本身没有标记为有日志，检查它的子节点
        if (!isCovered) {
            for (const auto& child : branchNode->children) {
                if (child->hasLogging) {
                    isCovered = true;
                    break;
                }
            }
        }

        // 如果仍然未覆盖，检查是否在logCalls中有记录
        if (!isCovered) {
            for (const auto& range : logLineRanges) {
                // 检查分支节点的行号是否在任何日志调用的范围内
                if (branchNode->location.line >= range.first && branchNode->location.line <= range.second) {
                    isCovered = true;
                    break;
                }
            }
        }

        if (!isCovered) {
            // 创建未覆盖路径信息
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::BRANCH;
            uncoveredPath.nodeType = branchNode->type;
            uncoveredPath.location = branchNode->location;
            uncoveredPath.name = getBranchName(branchNode);
            uncoveredPath.text = branchNode->text.substr(0, 100) + (branchNode->text.length() > 100 ? "..." : "");
            uncoveredPath.suggestion = generateSuggestion(CoverageType::BRANCH, branchNode->type);

            // 添加到未覆盖路径列表
            stats.uncoveredPaths.push_back(uncoveredPath);
        } else {
            stats.coveredBranches++;
        }
    }

    // 计算分支覆盖率
    if (stats.totalBranches > 0) {
        stats.branchCoverage = static_cast<double>(stats.coveredBranches) / stats.totalBranches;
    }

    LOG_DEBUG_FMT("分支覆盖率: %.2f%% (%d/%d)", stats.branchCoverage * 100, stats.coveredBranches, stats.totalBranches);
}

void CoverageCalculator::calculateExceptionCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                    CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 遍历AST节点树，收集异常处理节点
    std::vector<const ast_analyzer::ASTNodeInfo*> exceptionNodes;
    collectNodesByType(node, {ast_analyzer::NodeType::TRY_STMT, ast_analyzer::NodeType::CATCH_STMT}, exceptionNodes);

    LOG_DEBUG_FMT("文件 %s 中发现 %zu 个异常处理", filePath.c_str(), exceptionNodes.size());

    // 更新异常处理总数
    stats.totalExceptionHandlers += exceptionNodes.size();

    // 获取该文件的日志调用，重点关注WARNING和ERROR级别的日志
    const auto& logCalls = logIdentifier_.getLogCalls(filePath);

    // 过滤出警告和错误级别的日志调用
    std::vector<log_identifier::LogCallInfo> errorLogCalls;
    for (const auto& logCall : logCalls) {
        if (logCall.level == log_identifier::LogLevel::WARNING ||
            logCall.level == log_identifier::LogLevel::CRITICAL ||
            logCall.level == log_identifier::LogLevel::FATAL) {
            errorLogCalls.push_back(logCall);
        }
    }

    // 创建日志行号区间集合，用于判断异常处理是否覆盖
    std::vector<std::pair<unsigned int, unsigned int>> logLineRanges;
    for (const auto& logCall : errorLogCalls) {
        // 为每个日志调用创建一个小范围的区间，±3行
        unsigned int startLine = (logCall.location.line > 3) ? (logCall.location.line - 3) : 1;
        unsigned int endLine = logCall.location.line + 3;
        logLineRanges.push_back({startLine, endLine});
    }

    // 计算已覆盖的异常处理数量
    for (const auto* exceptionNode : exceptionNodes) {
        bool isCovered = exceptionNode->hasLogging;

        // 对于catch节点，覆盖要求更严格，应该在其内部有警告或错误级别的日志
        if (exceptionNode->type == ast_analyzer::NodeType::CATCH_STMT) {
            isCovered = false; // 重置，更严格要求

            // 检查是否有严重级别的日志调用在catch块内
            for (const auto& range : logLineRanges) {
                if (exceptionNode->location.line >= range.first && exceptionNode->location.line <= range.second) {
                    isCovered = true;
                    break;
                }
            }

            // 递归检查子节点
            for (const auto& child : exceptionNode->children) {
                if (child->hasLogging) {
                    isCovered = true;
                    break;
                }
            }
        }

        if (!isCovered) {
            // 创建未覆盖路径信息
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::EXCEPTION;
            uncoveredPath.nodeType = exceptionNode->type;
            uncoveredPath.location = exceptionNode->location;
            uncoveredPath.name =
                exceptionNode->type == ast_analyzer::NodeType::CATCH_STMT ? "catch " + exceptionNode->name : "try";
            uncoveredPath.text = exceptionNode->text.substr(0, 100) + (exceptionNode->text.length() > 100 ? "..." : "");
            uncoveredPath.suggestion = generateSuggestion(CoverageType::EXCEPTION, exceptionNode->type);

            // 添加到未覆盖路径列表
            stats.uncoveredPaths.push_back(uncoveredPath);
        } else {
            stats.coveredExceptionHandlers++;
        }
    }

    // 计算异常覆盖率
    if (stats.totalExceptionHandlers > 0) {
        stats.exceptionCoverage = static_cast<double>(stats.coveredExceptionHandlers) / stats.totalExceptionHandlers;
    }

    LOG_DEBUG_FMT("异常覆盖率: %.2f%% (%d/%d)", stats.exceptionCoverage * 100, stats.coveredExceptionHandlers,
                  stats.totalExceptionHandlers);
}

void CoverageCalculator::calculateKeyPathCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                  CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 我们定义关键路径包括：
    // 1. 所有函数入口
    // 2. 所有异常处理
    // 3. 重要的条件分支（例如，处理错误条件的分支）

    // 收集函数入口
    std::vector<const ast_analyzer::ASTNodeInfo*> functionEntries;
    collectNodesByType(node, {ast_analyzer::NodeType::FUNCTION, ast_analyzer::NodeType::METHOD}, functionEntries);

    // 收集异常处理
    std::vector<const ast_analyzer::ASTNodeInfo*> exceptionHandlers;
    collectNodesByType(node, {ast_analyzer::NodeType::CATCH_STMT}, exceptionHandlers);

    // 尝试识别关键分支
    std::vector<const ast_analyzer::ASTNodeInfo*> keyBranches;
    identifyKeyBranches(node, keyBranches);

    // 定义关键路径节点集合
    std::vector<const ast_analyzer::ASTNodeInfo*> keyPathNodes;
    keyPathNodes.insert(keyPathNodes.end(), functionEntries.begin(), functionEntries.end());
    keyPathNodes.insert(keyPathNodes.end(), exceptionHandlers.begin(), exceptionHandlers.end());
    keyPathNodes.insert(keyPathNodes.end(), keyBranches.begin(), keyBranches.end());

    LOG_DEBUG_FMT("文件 %s 中发现 %zu 个关键路径", filePath.c_str(), keyPathNodes.size());

    // 更新关键路径总数
    stats.totalKeyPaths += keyPathNodes.size();

    // 获取该文件的所有日志调用
    const auto& logCalls = logIdentifier_.getLogCalls(filePath);

    // 计算已覆盖的关键路径数量
    for (const auto* keyPathNode : keyPathNodes) {
        bool isCovered = keyPathNode->hasLogging;

        // 如果节点本身没有标记为有日志，检查它的子节点
        if (!isCovered) {
            for (const auto& child : keyPathNode->children) {
                if (child->hasLogging) {
                    isCovered = true;
                    break;
                }
            }
        }

        // 如果仍然未覆盖，检查是否在logCalls中有记录
        if (!isCovered) {
            for (const auto& logCall : logCalls) {
                // 检查日志调用是否在关键路径节点的范围内
                // 这是一个简化的检查，只比较行号
                if (logCall.location.line >= keyPathNode->location.line &&
                    logCall.location.line <= keyPathNode->location.line + 5) { // 假设关键路径节点不超过5行
                    isCovered = true;
                    break;
                }
            }
        }

        if (!isCovered) {
            // 创建未覆盖路径信息
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::KEY_PATH;
            uncoveredPath.nodeType = keyPathNode->type;
            uncoveredPath.location = keyPathNode->location;
            uncoveredPath.name = keyPathNode->name.empty() ? "未命名节点" : keyPathNode->name;
            uncoveredPath.text = keyPathNode->text.substr(0, 100) + (keyPathNode->text.length() > 100 ? "..." : "");
            uncoveredPath.suggestion = generateSuggestion(CoverageType::KEY_PATH, keyPathNode->type);

            // 添加到未覆盖路径列表
            stats.uncoveredPaths.push_back(uncoveredPath);
        } else {
            stats.coveredKeyPaths++;
        }
    }

    // 计算关键路径覆盖率
    if (stats.totalKeyPaths > 0) {
        stats.keyPathCoverage = static_cast<double>(stats.coveredKeyPaths) / stats.totalKeyPaths;
    }

    LOG_DEBUG_FMT("关键路径覆盖率: %.2f%% (%d/%d)", stats.keyPathCoverage * 100, stats.coveredKeyPaths,
                  stats.totalKeyPaths);
}

void CoverageCalculator::identifyKeyBranches(const ast_analyzer::ASTNodeInfo* node,
                                            std::vector<const ast_analyzer::ASTNodeInfo*>& keyBranches) {
    if (!node) {
        return;
    }

    // 识别可能是关键分支的条件语句
    // 这里使用一些启发式规则来判断分支是否关键
    if (node->type == ast_analyzer::NodeType::IF_STMT ||
        node->type == ast_analyzer::NodeType::ELSE_STMT) {

        // 通过文本内容判断是否为关键分支
        // 比如检查是否包含错误处理相关的关键词
        if (node->text.find("error") != std::string::npos ||
            node->text.find("fail") != std::string::npos ||
            node->text.find("exception") != std::string::npos ||
            node->text.find("return false") != std::string::npos ||
            node->text.find("return -1") != std::string::npos ||
            node->text.find("throw") != std::string::npos) {

            keyBranches.push_back(node);
        }
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        identifyKeyBranches(child.get(), keyBranches);
    }
}

void CoverageCalculator::mergeCoverageStats(CoverageStats& overall) {
    // 重置总体统计信息
    overall = CoverageStats();

    // 累加各文件的统计数据
    for (const auto& [filePath, stats] : coverageStats_) {
        overall.totalFunctions += stats.totalFunctions;
        overall.totalBranches += stats.totalBranches;
        overall.totalExceptionHandlers += stats.totalExceptionHandlers;
        overall.totalKeyPaths += stats.totalKeyPaths;

        overall.coveredFunctions += stats.coveredFunctions;
        overall.coveredBranches += stats.coveredBranches;
        overall.coveredExceptionHandlers += stats.coveredExceptionHandlers;
        overall.coveredKeyPaths += stats.coveredKeyPaths;

        // 合并未覆盖路径
        overall.uncoveredPaths.insert(overall.uncoveredPaths.end(), stats.uncoveredPaths.begin(),
                                      stats.uncoveredPaths.end());
    }

    // 计算总体覆盖率
    calculateOverallCoverage(overall);
}

void CoverageCalculator::collectNodesByType(const ast_analyzer::ASTNodeInfo* node,
                                            const std::vector<ast_analyzer::NodeType>& types,
                                            std::vector<const ast_analyzer::ASTNodeInfo*>& result) {
    if (!node) {
        return;
    }

    // 检查当前节点类型是否在目标类型列表中
    if (std::find(types.begin(), types.end(), node->type) != types.end()) {
        result.push_back(node);
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        collectNodesByType(child.get(), types, result);
    }
}

std::string CoverageCalculator::getBranchName(const ast_analyzer::ASTNodeInfo* node) {
    if (!node) {
        return "未知分支";
    }

    switch (node->type) {
        case ast_analyzer::NodeType::IF_STMT:
            return "if 分支";
        case ast_analyzer::NodeType::ELSE_STMT:
            return "else 分支";
        case ast_analyzer::NodeType::SWITCH_STMT:
            return "switch 语句";
        case ast_analyzer::NodeType::CASE_STMT:
            return "case 分支";
        case ast_analyzer::NodeType::FOR_STMT:
            return "for 循环";
        case ast_analyzer::NodeType::WHILE_STMT:
            return "while 循环";
        case ast_analyzer::NodeType::DO_STMT:
            return "do-while 循环";
        default:
            return "未知分支类型";
    }
}

void CoverageCalculator::calculateOverallCoverage(CoverageStats& stats) {
    // 计算综合覆盖率，各类型覆盖率的加权平均
    double totalWeight = 0.0;
    double weightedSum = 0.0;

    // 函数覆盖率（权重：1.0）
    if (stats.totalFunctions > 0) {
        weightedSum += stats.functionCoverage * 1.0;
        totalWeight += 1.0;
    }

    // 分支覆盖率（权重：1.5）
    if (stats.totalBranches > 0) {
        weightedSum += stats.branchCoverage * 1.5;
        totalWeight += 1.5;
    }

    // 异常覆盖率（权重：1.2）
    if (stats.totalExceptionHandlers > 0) {
        weightedSum += stats.exceptionCoverage * 1.2;
        totalWeight += 1.2;
    }

    // 关键路径覆盖率（权重：2.0）
    if (stats.totalKeyPaths > 0) {
        weightedSum += stats.keyPathCoverage * 2.0;
        totalWeight += 2.0;
    }

    // 计算加权平均
    if (totalWeight > 0) {
        stats.overallCoverage = weightedSum / totalWeight;
    } else {
        stats.overallCoverage = 0.0;
    }
}

std::string CoverageCalculator::generateSuggestion(CoverageType type, ast_analyzer::NodeType nodeType) const {
    std::stringstream suggestion;

    switch (type) {
        case CoverageType::FUNCTION:
            suggestion << "在函数入口添加调试级别日志，记录函数调用及其参数。";
            break;

        case CoverageType::BRANCH:
            switch (nodeType) {
                case ast_analyzer::NodeType::IF_STMT:
                    suggestion << "在if分支中添加适当级别的日志，记录条件判断结果和执行路径。";
                    break;
                case ast_analyzer::NodeType::ELSE_STMT:
                    suggestion << "在else分支中添加适当级别的日志，记录执行路径。";
                    break;
                case ast_analyzer::NodeType::SWITCH_STMT:
                    suggestion << "在switch语句的各个case中添加适当级别的日志，记录匹配的情况。";
                    break;
                case ast_analyzer::NodeType::FOR_STMT:
                case ast_analyzer::NodeType::WHILE_STMT:
                case ast_analyzer::NodeType::DO_STMT:
                    suggestion << "在循环开始、关键迭代点和结束时添加日志，记录循环状态和进度。";
                    break;
                default:
                    suggestion << "在此分支添加适当级别的日志，记录执行路径和关键状态。";
                    break;
            }
            break;

        case CoverageType::EXCEPTION:
            if (nodeType == ast_analyzer::NodeType::CATCH_STMT) {
                suggestion << "在catch块中添加警告或错误级别日志，记录异常信息和处理方式。";
            } else {
                suggestion << "在try块中添加适当级别的日志，记录可能发生异常的操作。";
            }
            break;

        case CoverageType::KEY_PATH:
            suggestion << "这是关键执行路径，应添加适当级别的日志，记录重要操作和状态变化。";
            break;

        default:
            suggestion << "添加适当级别的日志，记录执行情况和重要状态。";
            break;
    }

    return suggestion.str();
}

}  // namespace coverage
}  // namespace core
}  // namespace dlogcover
