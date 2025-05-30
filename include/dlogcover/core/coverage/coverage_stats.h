/**
 * @file coverage_stats.h
 * @brief 覆盖率统计信息
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_COVERAGE_COVERAGE_STATS_H
#define DLOGCOVER_CORE_COVERAGE_COVERAGE_STATS_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>
#include <vector>
#include <map>

namespace dlogcover {
namespace core {
namespace coverage {

/**
 * @brief 覆盖率类型枚举
 */
enum class CoverageType {
    FUNCTION = 0,       ///< 函数覆盖率
    BRANCH = 1,         ///< 分支覆盖率
    EXCEPTION = 2,      ///< 异常覆盖率
    KEY_PATH = 3        ///< 关键路径覆盖率
};

/**
 * @brief 未覆盖路径信息
 */
struct UncoveredPathInfo {
    CoverageType type;                          ///< 覆盖类型
    ast_analyzer::NodeType nodeType;           ///< 节点类型
    ast_analyzer::LocationInfo location;       ///< 位置信息
    std::string name;                           ///< 名称
    std::string text;                           ///< 文本内容
    std::string suggestion;                     ///< 建议
};

/**
 * @brief 函数覆盖率信息
 */
struct FunctionCoverageInfo {
    std::string name;           ///< 函数名
    std::string signature;      ///< 函数签名
    int startLine;              ///< 起始行号
    int endLine;                ///< 结束行号
    bool hasLogging;            ///< 是否有日志记录
    double coverageRatio;       ///< 覆盖率比例
};

/**
 * @brief 分支覆盖率信息
 */
struct BranchCoverageInfo {
    int line;                   ///< 行号
    std::string type;           ///< 分支类型（if, switch, loop等）
    bool hasLogging;            ///< 是否有日志记录
    int totalBranches;          ///< 总分支数
    int coveredBranches;        ///< 已覆盖分支数
};

/**
 * @brief 异常处理覆盖率信息
 */
struct ExceptionCoverageInfo {
    int line;                   ///< 行号
    std::string type;           ///< 异常处理类型（try-catch, throw等）
    bool hasLogging;            ///< 是否有日志记录
};

/**
 * @brief 关键路径覆盖率信息
 */
struct KeyPathCoverageInfo {
    std::string pathName;       ///< 路径名称
    std::vector<int> lines;     ///< 路径包含的行号
    bool hasLogging;            ///< 是否有日志记录
    double importance;          ///< 重要性评分
};

/**
 * @brief 文件覆盖率统计
 */
struct FileCoverageStats {
    std::string filePath;                                   ///< 文件路径
    std::string relativePath;                               ///< 相对路径
    
    // 函数覆盖率
    std::vector<FunctionCoverageInfo> functions;            ///< 函数列表
    int totalFunctions;                                     ///< 总函数数
    int coveredFunctions;                                   ///< 已覆盖函数数
    double functionCoverageRatio;                           ///< 函数覆盖率
    
    // 分支覆盖率
    std::vector<BranchCoverageInfo> branches;               ///< 分支列表
    int totalBranches;                                      ///< 总分支数
    int coveredBranches;                                    ///< 已覆盖分支数
    double branchCoverageRatio;                             ///< 分支覆盖率
    
    // 异常处理覆盖率
    std::vector<ExceptionCoverageInfo> exceptions;          ///< 异常处理列表
    int totalExceptions;                                    ///< 总异常处理数
    int coveredExceptions;                                  ///< 已覆盖异常处理数
    double exceptionCoverageRatio;                          ///< 异常处理覆盖率
    
    // 关键路径覆盖率
    std::vector<KeyPathCoverageInfo> keyPaths;              ///< 关键路径列表
    int totalKeyPaths;                                      ///< 总关键路径数
    int coveredKeyPaths;                                    ///< 已覆盖关键路径数
    double keyPathCoverageRatio;                            ///< 关键路径覆盖率
    
    // 总体覆盖率
    double overallCoverageRatio;                            ///< 总体覆盖率
};

/**
 * @brief 覆盖率统计信息
 */
struct CoverageStats {
    std::string projectName;                                ///< 项目名称
    std::string timestamp;                                  ///< 时间戳
    
    // 文件级统计
    std::vector<FileCoverageStats> files;                   ///< 文件统计列表
    int totalFiles;                                         ///< 总文件数
    int coveredFiles;                                       ///< 有覆盖的文件数
    
    // 总体统计
    int totalFunctions;                                     ///< 总函数数
    int coveredFunctions;                                   ///< 已覆盖函数数
    double functionCoverageRatio;                           ///< 函数覆盖率
    
    int totalBranches;                                      ///< 总分支数
    int coveredBranches;                                    ///< 已覆盖分支数
    double branchCoverageRatio;                             ///< 分支覆盖率
    
    int totalExceptions;                                    ///< 总异常处理数
    int coveredExceptions;                                  ///< 已覆盖异常处理数
    double exceptionCoverageRatio;                          ///< 异常处理覆盖率
    
    int totalKeyPaths;                                      ///< 总关键路径数
    int coveredKeyPaths;                                    ///< 已覆盖关键路径数
    double keyPathCoverageRatio;                            ///< 关键路径覆盖率
    
    double overallCoverageRatio;                            ///< 总体覆盖率
    
    // 兼容性成员 - 与cpp文件中的使用匹配
    double functionCoverage = 0.0;                          ///< 函数覆盖率（兼容）
    double branchCoverage = 0.0;                            ///< 分支覆盖率（兼容）
    double exceptionCoverage = 0.0;                         ///< 异常覆盖率（兼容）
    double keyPathCoverage = 0.0;                           ///< 关键路径覆盖率（兼容）
    double overallCoverage = 0.0;                           ///< 总体覆盖率（兼容）
    
    // cpp文件中使用的异常处理相关成员
    int totalExceptionHandlers = 0;                         ///< 总异常处理器数
    int coveredExceptionHandlers = 0;                       ///< 已覆盖异常处理器数
    
    // 未覆盖路径信息
    std::vector<UncoveredPathInfo> uncoveredPaths;          ///< 未覆盖路径列表
    
    /**
     * @brief 默认构造函数
     */
    CoverageStats() : totalFiles(0), coveredFiles(0), 
                     totalFunctions(0), coveredFunctions(0), functionCoverageRatio(0.0),
                     totalBranches(0), coveredBranches(0), branchCoverageRatio(0.0),
                     totalExceptions(0), coveredExceptions(0), exceptionCoverageRatio(0.0),
                     totalKeyPaths(0), coveredKeyPaths(0), keyPathCoverageRatio(0.0),
                     overallCoverageRatio(0.0) {}
    
    /**
     * @brief 清空统计信息
     */
    void clear() {
        files.clear();
        totalFiles = 0;
        coveredFiles = 0;
        totalFunctions = 0;
        coveredFunctions = 0;
        functionCoverageRatio = 0.0;
        functionCoverage = 0.0;
        totalBranches = 0;
        coveredBranches = 0;
        branchCoverageRatio = 0.0;
        branchCoverage = 0.0;
        totalExceptions = 0;
        coveredExceptions = 0;
        exceptionCoverageRatio = 0.0;
        exceptionCoverage = 0.0;
        totalExceptionHandlers = 0;
        coveredExceptionHandlers = 0;
        totalKeyPaths = 0;
        coveredKeyPaths = 0;
        keyPathCoverageRatio = 0.0;
        keyPathCoverage = 0.0;
        overallCoverageRatio = 0.0;
        overallCoverage = 0.0;
        uncoveredPaths.clear();
    }
};

} // namespace coverage
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_COVERAGE_COVERAGE_STATS_H 