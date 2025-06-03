/**
 * @file ast_expression_analyzer.h
 * @brief AST表达式分析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_EXPRESSION_ANALYZER_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_EXPRESSION_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/config/config.h>

// Clang headers
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Casting.h>

#include <memory>
#include <string>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST表达式分析器类
 * 
 * 专门用于分析表达式节点，特别是函数调用表达式
 */
class ASTExpressionAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param context AST上下文
     * @param filePath 文件路径
     * @param config 配置管理器
     */
    ASTExpressionAnalyzer(clang::ASTContext& context, 
                         const std::string& filePath,
                         const dlogcover::config::Config& config)
        : context_(context), filePath_(filePath), config_(config) {}

    /**
     * @brief 析构函数
     */
    ~ASTExpressionAnalyzer() = default;

    /**
     * @brief 分析表达式节点
     * @param expr 表达式节点
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeExpr(clang::Expr* expr);

    /**
     * @brief 分析函数调用表达式
     * @param call 函数调用表达式
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeCallExpr(clang::CallExpr* call);

    /**
     * @brief 检查是否为日志函数调用
     * @param call 函数调用表达式
     * @return 如果是日志函数调用返回true
     */
    bool isLogFunctionCall(clang::CallExpr* call) const;

private:
    clang::ASTContext& context_;                           ///< AST上下文
    std::string filePath_;                                 ///< 文件路径
    const dlogcover::config::Config& config_;      ///< 配置管理器

    /**
     * @brief 创建表达式节点信息
     * @param expr 表达式节点
     * @return 节点信息
     */
    std::unique_ptr<ASTNodeInfo> createExpressionNodeInfo(clang::Expr* expr);

    /**
     * @brief 获取表达式位置信息
     * @param expr 表达式节点
     * @return 位置信息
     */
    LocationInfo getExpressionLocation(clang::Expr* expr);

    /**
     * @brief 获取函数名称
     * @param expr 函数调用表达式
     * @return 函数名称
     */
    std::string getFunctionName(clang::CallExpr* expr) const;

    /**
     * @brief 创建节点信息
     * @param type 节点类型
     * @param name 节点名称
     * @param location 源码位置
     * @return 节点信息
     */
    std::unique_ptr<ASTNodeInfo> createNodeInfo(NodeType type, 
                                               const std::string& name,
                                               clang::SourceLocation location) {
        auto info = std::make_unique<ASTNodeInfo>();
        info->type = type;
        info->name = name;
        info->location = getLocation(location);
        return info;
    }

    /**
     * @brief 获取源码文本
     * @param startLoc 开始位置
     * @param endLoc 结束位置
     * @return 源码文本
     */
    std::string getSourceText(clang::SourceLocation startLoc, 
                             clang::SourceLocation endLoc) const {
        if (!startLoc.isValid() || !endLoc.isValid()) {
            return "";
        }
        
        auto& sourceManager = context_.getSourceManager();
        if (sourceManager.getFileID(startLoc) != sourceManager.getFileID(endLoc)) {
            return "";
        }
        
        clang::CharSourceRange range = clang::CharSourceRange::getTokenRange(startLoc, endLoc);
        if (range.isInvalid()) {
            return "";
        }
        
        bool invalid = false;
        llvm::StringRef text = clang::Lexer::getSourceText(range, sourceManager, context_.getLangOpts(), &invalid);
        if (invalid) {
            return "";
        }
        
        return text.str();
    }

    /**
     * @brief 获取位置信息
     * @param loc 源码位置
     * @return 位置信息
     */
    Location getLocation(clang::SourceLocation loc) const {
        Location location;
        if (!loc.isValid()) {
            return location;
        }
        
        auto& sourceManager = context_.getSourceManager();
        location.line = sourceManager.getSpellingLineNumber(loc);
        location.column = sourceManager.getSpellingColumnNumber(loc);
        
        return location;
    }
};

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_EXPRESSION_ANALYZER_H 