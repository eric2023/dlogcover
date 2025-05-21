/**
 * @file coverage_calculator.cpp
 * @brief 覆盖率计算器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 *
 * @note 重构说明：
 *   1. 移除了硬编码的日志函数/宏检测逻辑，改为使用配置文件中的logFunctions配置
 *   2. 添加了getAllLogFunctionsAndMacros辅助方法，用于获取配置中所有日志函数名称
 *   3. 优化了containsLogMacros方法，使用配置的日志函数集合进行检查
 *   4. 改进了hasLoggingInSubtree方法，移除硬编码的LOG_关键字检查
 *   5. 重构了identifyKeyBranches方法，增加了可扩展性
 *   6. 使用命名常量替代硬编码的错误关键词
 */

#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace dlogcover {
namespace core {
namespace coverage {

// 关键词列表，用于识别错误处理相关的分支
// 这些关键词也可以通过配置文件提供，增加灵活性
const std::vector<std::string> CoverageCalculator::ERROR_KEYWORDS = {
    "error", "fail", "exception", "return false", "return -1", "throw", "failed", "invalid", "nullptr", "NULL"};

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

    // 添加验证步骤，检查是否存在零覆盖率的异常情况
    if (overallStats_.totalFunctions == 0 && overallStats_.totalBranches == 0 &&
        overallStats_.totalExceptionHandlers == 0 && overallStats_.totalKeyPaths == 0) {
        LOG_WARNING("警告：未检测到任何代码元素（函数、分支、异常处理或关键路径）");
        LOG_WARNING("这可能表示AST分析过程出现问题或配置中的包含/排除规则设置不正确");

        // 打印配置信息以帮助诊断
        LOG_WARNING_FMT("扫描目录: %zu 个", config_.scan.directories.size());
        for (const auto& dir : config_.scan.directories) {
            LOG_WARNING_FMT("  - %s", dir.c_str());
        }

        LOG_WARNING_FMT("排除目录: %zu 个", config_.scan.excludes.size());
        for (const auto& exclude : config_.scan.excludes) {
            LOG_WARNING_FMT("  - %s", exclude.c_str());
        }

        LOG_WARNING_FMT("文件类型: %zu 个", config_.scan.fileTypes.size());
        for (const auto& type : config_.scan.fileTypes) {
            LOG_WARNING_FMT("  - %s", type.c_str());
        }

        LOG_WARNING_FMT("检查到的AST节点数量: %zu", astAnalyzer_.getAllASTNodeInfo().size());

        // 尽管出现异常情况，依然返回true，让流程继续，以便生成报告
    }

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
    LOG_DEBUG_FMT("文件 %s 中发现 %zu 个日志调用", filePath.c_str(), logCalls.size());

    // 创建日志行号集合，用于快速查找
    std::unordered_set<unsigned int> logLines;
    for (const auto& logCall : logCalls) {
        logLines.insert(logCall.location.line);
        LOG_DEBUG_FMT("日志调用行号: %u, 函数: %s", logCall.location.line, logCall.functionName.c_str());
    }

    // 计算已覆盖的函数数量
    for (const auto* funcNode : functionNodes) {
        // 跳过复合语句节点，确保只处理真正的函数
        if (funcNode->name == "compound") {
            continue;
        }

        // 首先检查函数节点本身是否已标记为有日志调用
        bool isCovered = funcNode->hasLogging;
        LOG_DEBUG_FMT("函数 %s 在位置 %s:%u hasLogging=%d", funcNode->name.c_str(), funcNode->location.filePath.c_str(),
                      funcNode->location.line, isCovered);

        // 增强的覆盖检测: 检查函数的子节点是否含有日志调用
        if (!isCovered) {
            isCovered = containsLogMacros(funcNode);
            if (isCovered) {
                LOG_DEBUG_FMT("函数 %s 在子节点中包含配置的日志函数或宏调用", funcNode->name.c_str());
            }
        }

        // 如果仍然未覆盖，检查是否在logCalls中有记录
        if (!isCovered) {
            // 获取函数的起始和结束行
            unsigned int funcStartLine = funcNode->location.line;
            unsigned int funcEndLine = funcNode->location.line;

            // 确定函数的结束行
            if (funcNode->endLocation.line > 0) {
                funcEndLine = funcNode->endLocation.line;
            } else {
                // 如果没有确切的结束行，给一个合理的范围
                // 为提高准确度，尝试从函数文本中推导结束行号
                size_t braceCount = 0;
                size_t lineCount = 0;
                std::istringstream stream(funcNode->text);
                std::string line;

                while (std::getline(stream, line)) {
                    lineCount++;
                    for (char c : line) {
                        if (c == '{')
                            braceCount++;
                        if (c == '}')
                            braceCount--;
                    }
                }

                // 更新函数结束行
                if (lineCount > 0) {
                    funcEndLine = funcStartLine + lineCount - 1;
                }
            }

            // 检查函数范围内是否有日志调用
            for (unsigned int line = funcStartLine; line <= funcEndLine; ++line) {
                if (logLines.count(line) > 0) {
                    isCovered = true;
                    LOG_DEBUG_FMT("函数 %s 在行 %u 有日志调用", funcNode->name.c_str(), line);
                    break;
                }
            }
        }

        // 使用递归方式检查子节点的日志调用
        if (!isCovered && hasLoggingInSubtree(funcNode)) {
            isCovered = true;
            LOG_DEBUG_FMT("函数 %s 在子节点中有日志调用", funcNode->name.c_str());
        }

        // 更新覆盖状态
        if (isCovered) {
            stats.coveredFunctions++;
            LOG_DEBUG_FMT("函数 %s 被覆盖", funcNode->name.c_str());
        } else {
            LOG_DEBUG_FMT("函数 %s 未被覆盖", funcNode->name.c_str());

            // 添加未覆盖路径信息
            UncoveredPathInfo uncoveredPath;
            uncoveredPath.type = CoverageType::FUNCTION;
            uncoveredPath.nodeType = funcNode->type;
            uncoveredPath.location = funcNode->location;
            uncoveredPath.name = funcNode->name;
            uncoveredPath.text = funcNode->text.substr(0, 100);  // 只取前100个字符
            uncoveredPath.suggestion = generateSuggestion(CoverageType::FUNCTION, funcNode->type);

            stats.uncoveredPaths.push_back(std::move(uncoveredPath));
        }
    }

    // 计算函数覆盖率
    if (stats.totalFunctions > 0) {
        stats.functionCoverage = static_cast<double>(stats.coveredFunctions) / stats.totalFunctions;
    } else {
        stats.functionCoverage = 0.0;
    }

    LOG_DEBUG_FMT("文件 %s 的函数覆盖率: %.2f%% (%d/%d)", filePath.c_str(), stats.functionCoverage * 100,
                  stats.coveredFunctions, stats.totalFunctions);
}

bool CoverageCalculator::containsLogMacros(const ast_analyzer::ASTNodeInfo* node) const {
    if (!node) {
        return false;
    }

    // 获取所有配置的日志函数和宏名称
    const auto allLogFunctions = getAllLogFunctionsAndMacros();

    // 检查节点文本中是否包含任何配置的日志函数或宏调用
    if (!node->text.empty()) {
        // 检查节点文本是否包含任何已配置的日志函数或宏
        // 注意：这是一个简单的文本匹配，不如AST分析精确，但可以捕获更多潜在的日志调用
        for (const auto& logFunction : allLogFunctions) {
            if (node->text.find(logFunction) != std::string::npos) {
                LOG_DEBUG_FMT("节点文本中包含日志函数/宏调用: %s", logFunction.c_str());
                return true;
            }
        }
    }

    // 对于调用表达式，检查是否是日志函数调用
    if (node->type == ast_analyzer::NodeType::CALL_EXPR || node->type == ast_analyzer::NodeType::LOG_CALL_EXPR) {
        // 判断函数名是否在日志函数列表中
        if (allLogFunctions.find(node->name) != allLogFunctions.end()) {
            LOG_DEBUG_FMT("节点是已知的日志函数调用: %s", node->name.c_str());
            return true;
        }
    }

    // 递归检查子节点
    for (const auto& child : node->children) {
        if (containsLogMacros(child.get())) {
            return true;
        }
    }

    return false;
}

bool CoverageCalculator::hasLoggingInSubtree(const ast_analyzer::ASTNodeInfo* node) const {
    if (!node) {
        return false;
    }

    // 首先检查节点本身是否包含日志调用
    if (node->hasLogging) {
        LOG_DEBUG_FMT("节点 %s 本身包含日志调用", node->name.c_str());
        return true;
    }

    // 使用containsLogMacros方法检查是否包含配置中定义的任何日志函数或宏
    if (containsLogMacros(node)) {
        LOG_DEBUG_FMT("节点 %s 包含配置的日志函数或宏调用", node->name.c_str());
        return true;
    }

    // 递归检查子节点
    for (const auto& child : node->children) {
        if (hasLoggingInSubtree(child.get())) {
            LOG_DEBUG_FMT("节点 %s 的子节点包含日志调用", node->name.c_str());
            return true;
        }
    }

    return false;
}

void CoverageCalculator::calculateBranchCoverage(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath,
                                                 CoverageStats& stats) {
    if (!node) {
        return;
    }

    // 遍历AST节点树，收集分支节点
    std::vector<const ast_analyzer::ASTNodeInfo*> branchNodes;
    collectNodesByType(node,
                       {ast_analyzer::NodeType::IF_STMT, ast_analyzer::NodeType::ELSE_STMT,
                        ast_analyzer::NodeType::SWITCH_STMT, ast_analyzer::NodeType::CASE_STMT},
                       branchNodes);

    LOG_DEBUG_FMT("文件 %s 中发现 %zu 个分支", filePath.c_str(), branchNodes.size());

    // 更新分支总数
    stats.totalBranches += branchNodes.size();

    // 获取该文件的日志调用
    const auto& logCalls = logIdentifier_.getLogCalls(filePath);

    // 创建日志行号集合，用于快速查找
    std::unordered_set<unsigned int> logLines;
    for (const auto& logCall : logCalls) {
        logLines.insert(logCall.location.line);
    }

    // 计算已覆盖的分支数量
    for (const auto* branchNode : branchNodes) {
        bool isCovered = branchNode->hasLogging;

        // 如果仍然未覆盖，检查是否在logCalls中有记录
        if (!isCovered) {
            // 获取分支的起始和结束行
            unsigned int branchStartLine = branchNode->location.line;

            // 结束行需要推断，这里假设分支语句不会太长
            unsigned int branchEndLine = branchStartLine + 20;  // 假设分支代码块不超过20行
            for (const auto& logCall : logCalls) {
                if (logCall.location.line >= branchStartLine && logCall.location.line <= branchEndLine &&
                    logCall.location.filePath == branchNode->location.filePath) {
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

    // 获取该文件的日志调用
    const auto& logCalls = logIdentifier_.getLogCalls(filePath);

    // 创建日志行号集合，用于快速查找
    std::unordered_set<unsigned int> logLines;
    for (const auto& logCall : logCalls) {
        logLines.insert(logCall.location.line);
    }

    // 计算已覆盖的异常处理数量
    for (const auto* exceptionNode : exceptionNodes) {
        bool isCovered = exceptionNode->hasLogging;

        // 如果仍然未覆盖，检查是否在logCalls中有记录
        if (!isCovered) {
            // 获取异常处理的起始和结束行
            unsigned int exceptionStartLine = exceptionNode->location.line;

            // 结束行需要推断，这里假设异常处理代码块不会太长
            unsigned int exceptionEndLine = exceptionStartLine + 20;  // 假设异常处理代码块不超过20行
            for (const auto& logCall : logCalls) {
                if (logCall.location.line >= exceptionStartLine && logCall.location.line <= exceptionEndLine &&
                    logCall.location.filePath == exceptionNode->location.filePath) {
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
            uncoveredPath.name = (exceptionNode->type == ast_analyzer::NodeType::TRY_STMT) ? "try" : "catch";
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
                    logCall.location.line <= keyPathNode->location.line + 5) {  // 假设关键路径节点不超过5行
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
    if (node->type == ast_analyzer::NodeType::IF_STMT || node->type == ast_analyzer::NodeType::ELSE_STMT) {
        // 通过文本内容判断是否为关键分支
        for (const auto& keyword : ERROR_KEYWORDS) {
            if (node->text.find(keyword) != std::string::npos) {
                keyBranches.push_back(node);
                break;
            }
        }
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        identifyKeyBranches(child.get(), keyBranches);
    }
}

void CoverageCalculator::mergeCoverageStats(CoverageStats& overall) {
    // 重置总体统计数据
    overall.totalFunctions = 0;
    overall.totalBranches = 0;
    overall.totalExceptionHandlers = 0;
    overall.totalKeyPaths = 0;
    overall.coveredFunctions = 0;
    overall.coveredBranches = 0;
    overall.coveredExceptionHandlers = 0;
    overall.coveredKeyPaths = 0;
    overall.uncoveredPaths.clear();

    // 合并所有文件的覆盖率统计
    for (const auto& [filePath, stats] : coverageStats_) {
        overall.totalFunctions += stats.totalFunctions;
        overall.totalBranches += stats.totalBranches;
        overall.totalExceptionHandlers += stats.totalExceptionHandlers;
        overall.totalKeyPaths += stats.totalKeyPaths;
        overall.coveredFunctions += stats.coveredFunctions;
        overall.coveredBranches += stats.coveredBranches;
        overall.coveredExceptionHandlers += stats.coveredExceptionHandlers;
        overall.coveredKeyPaths += stats.coveredKeyPaths;

        // 合并未覆盖路径列表
        overall.uncoveredPaths.insert(overall.uncoveredPaths.end(), stats.uncoveredPaths.begin(),
                                      stats.uncoveredPaths.end());
    }

    // 计算总体覆盖率
    if (overall.totalFunctions > 0) {
        overall.functionCoverage = static_cast<double>(overall.coveredFunctions) / overall.totalFunctions;
    }

    if (overall.totalBranches > 0) {
        overall.branchCoverage = static_cast<double>(overall.coveredBranches) / overall.totalBranches;
    }

    if (overall.totalExceptionHandlers > 0) {
        overall.exceptionCoverage =
            static_cast<double>(overall.coveredExceptionHandlers) / overall.totalExceptionHandlers;
    }

    if (overall.totalKeyPaths > 0) {
        overall.keyPathCoverage = static_cast<double>(overall.coveredKeyPaths) / overall.totalKeyPaths;
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
        // 对于FUNCTION和METHOD类型节点，确保不收集compound语句节点
        if ((node->type == ast_analyzer::NodeType::FUNCTION || node->type == ast_analyzer::NodeType::METHOD) &&
            node->name == "compound") {
            // 跳过复合语句块节点，它们不是真正的函数或方法
            LOG_DEBUG_FMT("跳过复合语句节点: %s", node->name.c_str());
        } else {
            result.push_back(node);
        }
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
    // 计算总体覆盖率，考虑各种类型覆盖率的权重
    double totalWeight = 0.0;
    double weightedSum = 0.0;

    // 函数覆盖率权重
    if (stats.totalFunctions > 0) {
        weightedSum += stats.functionCoverage * 0.4;
        totalWeight += 0.4;
    }

    // 分支覆盖率权重
    if (stats.totalBranches > 0) {
        weightedSum += stats.branchCoverage * 0.3;
        totalWeight += 0.3;
    }

    // 异常覆盖率权重
    if (stats.totalExceptionHandlers > 0) {
        weightedSum += stats.exceptionCoverage * 0.15;
        totalWeight += 0.15;
    }

    // 关键路径覆盖率权重
    if (stats.totalKeyPaths > 0) {
        weightedSum += stats.keyPathCoverage * 0.15;
        totalWeight += 0.15;
    }

    // 计算加权平均覆盖率
    if (totalWeight > 0) {
        stats.overallCoverage = weightedSum / totalWeight;
    } else {
        stats.overallCoverage = 0.0;
    }

    LOG_DEBUG_FMT("总体覆盖率: %.2f%%", stats.overallCoverage * 100);
}

std::string CoverageCalculator::generateSuggestion(CoverageType type, ast_analyzer::NodeType nodeType) const {
    // 根据覆盖率类型和节点类型生成建议
    switch (type) {
        case CoverageType::FUNCTION:
            return "在函数入口添加日志记录，记录参数和调用上下文信息";

        case CoverageType::BRANCH:
            switch (nodeType) {
                case ast_analyzer::NodeType::IF_STMT:
                    return "在if条件分支中添加日志记录，记录条件判断结果和关键变量值";
                case ast_analyzer::NodeType::ELSE_STMT:
                    return "在else分支中添加日志记录，记录进入else分支的信息和关键变量值";
                case ast_analyzer::NodeType::SWITCH_STMT:
                    return "在switch语句中添加日志记录，记录switch变量的值";
                case ast_analyzer::NodeType::CASE_STMT:
                    return "在case分支中添加日志记录，记录进入该case分支的信息";
                default:
                    return "在分支中添加适当的日志记录";
            }

        case CoverageType::EXCEPTION:
            switch (nodeType) {
                case ast_analyzer::NodeType::TRY_STMT:
                    return "在try块中关键位置添加日志记录，记录可能导致异常的操作";
                case ast_analyzer::NodeType::CATCH_STMT:
                    return "在catch块中添加日志记录，记录捕获到的异常信息和相关上下文";
                default:
                    return "在异常处理中添加适当的日志记录";
            }

        case CoverageType::KEY_PATH:
            switch (nodeType) {
                case ast_analyzer::NodeType::IF_STMT:
                    return "此if语句被识别为关键路径，应添加日志记录异常状态和处理过程";
                case ast_analyzer::NodeType::ELSE_STMT:
                    return "此else语句被识别为关键路径，应添加日志记录异常状态和处理过程";
                case ast_analyzer::NodeType::CATCH_STMT:
                    return "此catch块是关键异常处理路径，应详细记录异常信息和处理步骤";
                default:
                    return "在此关键路径中添加详细的日志记录，记录状态和处理过程";
            }

        default:
            return "添加适当的日志记录，提高代码可观测性";
    }
}

/**
 * @brief 获取配置中所有日志函数和宏的名称列表
 * @return 所有日志函数和宏名称的集合
 */
std::unordered_set<std::string> CoverageCalculator::getAllLogFunctionsAndMacros() const {
    std::unordered_set<std::string> result;

    // 获取LogIdentifier中已经解析的日志函数名集合
    const auto& logFunctionNames = logIdentifier_.getLogFunctionNames();
    result.insert(logFunctionNames.begin(), logFunctionNames.end());

    // 从配置中添加Qt日志函数
    if (config_.logFunctions.qt.enabled) {
        for (const auto& func : config_.logFunctions.qt.functions) {
            result.insert(func);
        }

        for (const auto& func : config_.logFunctions.qt.categoryFunctions) {
            result.insert(func);
        }
    }

    // 从配置中添加自定义日志函数
    if (config_.logFunctions.custom.enabled) {
        for (const auto& [level, funcs] : config_.logFunctions.custom.functions) {
            for (const auto& func : funcs) {
                result.insert(func);
            }
        }
    }

    return result;
}

}  // namespace coverage
}  // namespace core
}  // namespace dlogcover
