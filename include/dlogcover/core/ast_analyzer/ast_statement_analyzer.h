/**
 * @file ast_statement_analyzer.h
 * @brief AST语句分析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_STATEMENT_ANALYZER_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_STATEMENT_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/config/config.h>

// Clang headers
#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

#include <memory>
#include <string>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST语句分析器类
 * 
 * 负责分析各种语句节点类型
 */
class ASTStatementAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param context AST上下文
     * @param filePath 文件路径
     * @param config 配置管理器
     */
    ASTStatementAnalyzer(clang::ASTContext& context, 
                        const std::string& filePath,
                        const dlogcover::config::Config& config)
        : context_(context), filePath_(filePath), config_(config) {}

    /**
     * @brief 析构函数
     */
    ~ASTStatementAnalyzer() = default;

    /**
     * @brief 分析语句节点
     * @param stmt 语句节点
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeStmt(clang::Stmt* stmt);

    /**
     * @brief 分析复合语句
     * @param stmt 复合语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeCompoundStmt(clang::CompoundStmt* stmt);

    /**
     * @brief 分析if语句
     * @param stmt if语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeIfStmt(clang::IfStmt* stmt);

    /**
     * @brief 分析switch语句
     * @param stmt switch语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeSwitchStmt(clang::SwitchStmt* stmt);

    /**
     * @brief 分析for语句
     * @param stmt for语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeForStmt(clang::ForStmt* stmt);

    /**
     * @brief 分析while语句
     * @param stmt while语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeWhileStmt(clang::WhileStmt* stmt);

    /**
     * @brief 分析do语句
     * @param stmt do语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeDoStmt(clang::DoStmt* stmt);

    /**
     * @brief 分析try语句
     * @param stmt try语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeTryStmt(clang::CXXTryStmt* stmt);

    /**
     * @brief 分析catch语句
     * @param stmt catch语句
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeCatchStmt(clang::CXXCatchStmt* stmt);

private:
    clang::ASTContext& context_;                           ///< AST上下文
    std::string filePath_;                                 ///< 文件路径
    const dlogcover::config::Config& config_;      ///< 配置管理器

    /**
     * @brief 检查文本是否包含日志关键字
     * @param text 文本内容
     * @return 如果包含日志关键字返回true
     */
    bool containsLogKeywords(const std::string& text) const;

    /**
     * @brief 创建节点信息
     * @param type 节点类型
     * @param name 节点名称
     * @param location 源码位置
     * @param text 可选的文本内容
     * @return 节点信息
     */
    std::unique_ptr<ASTNodeInfo> createNodeInfo(NodeType type, 
                                               const std::string& name,
                                               clang::SourceLocation location,
                                               const std::string& text = "") {
        auto info = std::make_unique<ASTNodeInfo>();
        info->type = type;
        info->name = name;
        info->location = getLocation(location);
        info->text = text;
        return info;
    }

    /**
     * @brief 获取源码文本
     * @param startLoc 开始位置
     * @param endLoc 结束位置
     * @param maxLength 最大长度
     * @return 源码文本
     */
    std::string getSourceText(clang::SourceLocation startLoc, 
                             clang::SourceLocation endLoc,
                             size_t maxLength = 1000) const {
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
        
        std::string result = text.str();
        if (result.length() > maxLength) {
            result = result.substr(0, maxLength) + "...";
        }
        
        return result;
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

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_STATEMENT_ANALYZER_H 