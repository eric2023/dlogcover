/**
 * @file ast_analyzer.h
 * @brief AST分析器主类
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_ANALYZER_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/core/ast_analyzer/ast_function_analyzer.h>
#include <dlogcover/core/ast_analyzer/ast_expression_analyzer.h>
#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>

// Clang headers
#include <clang/AST/ASTContext.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/AST/Expr.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST分析器主类
 * 
 * 负责解析C++源代码的AST并提取日志相关信息
 */
class ASTAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param config 配置管理器
     * @param sourceManager 源文件管理器
     */
    ASTAnalyzer(const config::Config& config, const source_manager::SourceManager& sourceManager);

    /**
     * @brief 析构函数
     */
    ~ASTAnalyzer();

    /**
     * @brief 分析指定文件
     * @param filePath 文件路径
     * @return 分析结果
     */
    Result<bool> analyze(const std::string& filePath);

    /**
     * @brief 分析所有文件
     * @return 分析结果
     */
    Result<bool> analyzeAll();

    /**
     * @brief 获取分析结果
     * @return AST节点信息列表
     */
    const std::vector<std::unique_ptr<ASTNodeInfo>>& getResults() const;

    /**
     * @brief 清空分析结果
     */
    void clear();

    /**
     * @brief 获取指定文件的AST节点信息
     * @param filePath 文件路径
     * @return AST节点信息指针，如果不存在返回nullptr
     */
    const ASTNodeInfo* getASTNodeInfo(const std::string& filePath) const;

    /**
     * @brief 获取所有文件的AST节点信息
     * @return 所有文件的AST节点信息映射表的常量引用
     */
    const std::unordered_map<std::string, std::unique_ptr<ASTNodeInfo>>& getAllASTNodeInfo() const;

private:
    const config::Config& config_;                  ///< 配置管理器
    const source_manager::SourceManager& sourceManager_;   ///< 源文件管理器
    std::vector<std::unique_ptr<ASTNodeInfo>> results_;    ///< 分析结果
    std::unique_ptr<ASTFunctionAnalyzer> functionAnalyzer_;  ///< 函数分析器
    std::unique_ptr<ASTExpressionAnalyzer> expressionAnalyzer_;  ///< 表达式分析器
    
    // cpp文件中使用的成员变量
    std::unique_ptr<clang::ASTUnit> currentASTUnit_;        ///< 当前AST单元
    std::unordered_map<std::string, std::unique_ptr<ASTNodeInfo>> astNodes_;  ///< AST节点缓存

    /**
     * @brief 创建AST单元
     * @param filePath 文件路径
     * @param content 文件内容
     * @return AST单元
     */
    std::unique_ptr<clang::ASTUnit> createASTUnit(const std::string& filePath, 
                                                  const std::string& content);

    /**
     * @brief 分析AST上下文
     * @param context AST上下文
     * @param filePath 文件路径
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeASTContext(clang::ASTContext& context, 
                                                           const std::string& filePath);

    /**
     * @brief 检查是否为日志函数调用
     * @param call 函数调用表达式
     * @return 如果是日志函数调用返回true
     */
    bool isLogFunctionCall(clang::CallExpr* call) const;

    /**
     * @brief 递归分析命名空间的辅助函数
     * @param namespaceDecl 命名空间声明
     * @param namespacePath 命名空间路径（如 "dlogcover::core::ast_analyzer"）
     * @param filePath 文件路径
     * @param sourceManager 源代码管理器
     * @param functionAnalyzer 函数分析器
     * @param rootNode 根节点用于添加分析结果
     */
    void analyzeNamespaceRecursively(clang::NamespaceDecl* namespaceDecl, 
                                    const std::string& namespacePath,
                                    const std::string& filePath,
                                    clang::SourceManager& sourceManager,
                                    ASTFunctionAnalyzer& functionAnalyzer,
                                    std::unique_ptr<ASTNodeInfo>& rootNode);

    /**
     * @brief 获取编译参数
     * @return 编译参数列表
     */
    std::vector<std::string> getCompilerArgs() const;
};

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_ANALYZER_H 