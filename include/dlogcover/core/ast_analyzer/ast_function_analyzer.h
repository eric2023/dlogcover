/**
 * @file ast_function_analyzer.h
 * @brief AST函数分析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_FUNCTION_ANALYZER_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_FUNCTION_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/core/ast_analyzer/ast_node_analyzer.h>
#include <dlogcover/core/ast_analyzer/ast_statement_analyzer.h>
#include <dlogcover/core/ast_analyzer/ast_expression_analyzer.h>
#include <dlogcover/config/config.h>

// Clang headers
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>

#include <memory>
#include <string>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST函数分析器类
 * 
 * 专门用于分析函数声明和方法声明，提取日志相关信息
 */
class ASTFunctionAnalyzer : public ASTNodeAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param context AST上下文
     * @param filePath 文件路径
     * @param config 配置管理器
     */
    ASTFunctionAnalyzer(clang::ASTContext& context, 
                       const std::string& filePath,
                       const dlogcover::config::Config& config);

    /**
     * @brief 析构函数
     */
    ~ASTFunctionAnalyzer() = default;

    /**
     * @brief 分析函数声明
     * @param funcDecl 函数声明
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeFunctionDecl(clang::FunctionDecl* funcDecl);

    /**
     * @brief 分析方法声明
     * @param method 方法声明
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeMethodDecl(clang::CXXMethodDecl* method);

private:
    ASTStatementAnalyzer stmtAnalyzer_;                    ///< 语句分析器
    ASTExpressionAnalyzer exprAnalyzer_;                   ///< 表达式分析器

    /**
     * @brief 分析函数体
     * @param body 函数体语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeFunctionBody(clang::Stmt* body);
};

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_FUNCTION_ANALYZER_H 