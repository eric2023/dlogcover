/**
 * @file ast_analyzer.h
 * @brief AST分析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_ANALYZER_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/core/ast_analyzer/ast_function_analyzer.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/source_manager/source_manager.h>
#include <string>
#include <unordered_map>
#include <memory>

// 前向声明
namespace clang {
class ASTUnit;
class ASTContext;
class CallExpr;
}

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST分析器类
 * 
 * 负责分析C++源代码的抽象语法树，识别函数、分支、循环等结构
 */
class ASTAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param config 配置管理器引用
     * @param sourceManager 源文件管理器引用
     */
    ASTAnalyzer(const config::ConfigManager& config, const source_manager::SourceManager& sourceManager);

    /**
     * @brief 析构函数
     */
    ~ASTAnalyzer();

    /**
     * @brief 分析指定文件的AST
     * @param filePath 文件路径
     * @return 分析结果，成功返回true，否则返回错误信息
     */
    Result<bool> analyze(const std::string& filePath);

    /**
     * @brief 分析所有源文件的AST
     * @return 分析结果，成功返回true，否则返回错误信息
     */
    Result<bool> analyzeAll();

    /**
     * @brief 获取指定文件的AST节点信息
     * @param filePath 文件路径
     * @return AST节点信息指针，未找到返回nullptr
     */
    const ASTNodeInfo* getASTNodeInfo(const std::string& filePath) const;

    /**
     * @brief 获取所有文件的AST节点信息
     * @return AST节点信息映射表的常量引用
     */
    const std::unordered_map<std::string, std::unique_ptr<ASTNodeInfo>>& getAllASTNodeInfo() const;

private:
    const config::ConfigManager& config_;                                       ///< 配置管理器引用
    const source_manager::SourceManager& sourceManager_;                        ///< 源文件管理器引用
    std::unordered_map<std::string, std::unique_ptr<ASTNodeInfo>> astNodes_;    ///< AST节点信息映射表
    std::unique_ptr<clang::ASTUnit> currentASTUnit_;                            ///< 当前AST单元

    /**
     * @brief 创建AST单元
     * @param filePath 文件路径
     * @param content 文件内容
     * @return AST单元智能指针
     */
    std::unique_ptr<clang::ASTUnit> createASTUnit(const std::string& filePath, const std::string& content);

    /**
     * @brief 分析AST上下文
     * @param context AST上下文引用
     * @param filePath 文件路径
     * @return 分析结果
     */
    Result<std::unique_ptr<ASTNodeInfo>> analyzeASTContext(clang::ASTContext& context, const std::string& filePath);

    /**
     * @brief 判断是否为日志函数调用
     * @param expr 调用表达式指针
     * @return 如果是日志函数调用返回true，否则返回false
     */
    bool isLogFunctionCall(clang::CallExpr* expr) const;
};

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_ANALYZER_H 