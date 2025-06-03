/**
 * @file ast_node_analyzer.h
 * @brief AST节点分析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_NODE_ANALYZER_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_NODE_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/config/config.h>

// Clang headers
#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include <memory>
#include <string>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST节点分析器类
 * 
 * 负责分析各种AST节点类型
 */
class ASTNodeAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param context AST上下文
     * @param filePath 文件路径
     * @param config 配置管理器
     */
    ASTNodeAnalyzer(clang::ASTContext& context, 
                   const std::string& filePath,
                   const dlogcover::config::Config& config)
        : context_(context), filePath_(filePath), config_(&config) {}

    /**
     * @brief 构造函数（用于继承）
     * @param context AST上下文
     * @param filePath 文件路径
     */
    ASTNodeAnalyzer(clang::ASTContext& context, 
                   const std::string& filePath)
        : context_(context), filePath_(filePath), config_(nullptr) {}

    /**
     * @brief 析构函数
     */
    ~ASTNodeAnalyzer() = default;

    /**
     * @brief 分析语句节点
     * @param stmt 语句节点
     * @return 分析结果
     */
    virtual Result<std::unique_ptr<ASTNodeInfo>> analyzeStmt(clang::Stmt* stmt) {
        // 默认实现，返回空节点
        return makeSuccess(std::unique_ptr<ASTNodeInfo>(nullptr));
    }

    /**
     * @brief 分析声明节点
     * @param decl 声明节点
     * @return 分析结果
     */
    virtual Result<std::unique_ptr<ASTNodeInfo>> analyzeDecl(clang::Decl* decl) {
        // 默认实现，返回空节点
        return makeSuccess(std::unique_ptr<ASTNodeInfo>(nullptr));
    }

    /**
     * @brief 获取位置信息
     * @param loc 源码位置
     * @return 位置信息
     */
    Location getLocation(clang::SourceLocation loc) const;

    /**
     * @brief 创建节点信息（用于cpp文件中的实现）
     * @param type 节点类型
     * @param name 节点名称
     * @param location 源码位置
     * @param text 可选的文本内容
     * @return 节点信息
     */
    std::unique_ptr<ASTNodeInfo> createNodeInfo(NodeType type, 
                                               const std::string& name,
                                               clang::SourceLocation location,
                                               const std::string& text = "") const;

protected:
    /**
     * @brief 获取源码文本
     * @param startLoc 开始位置
     * @param endLoc 结束位置
     * @param maxLength 最大长度
     * @return 源码文本
     */
    std::string getSourceText(clang::SourceLocation startLoc, 
                             clang::SourceLocation endLoc,
                             size_t maxLength = 1000) const;

private:
    clang::ASTContext& context_;                           ///< AST上下文
    std::string filePath_;                                 ///< 文件路径
    const dlogcover::config::Config* config_;             ///< 配置管理器指针

    /**
     * @brief 创建节点信息
     * @param type 节点类型
     * @param name 节点名称
     * @param location 位置信息
     * @return 节点信息
     */
    std::unique_ptr<ASTNodeInfo> createNodeInfo(NodeType type, 
                                               const std::string& name,
                                               const LocationInfo& location);
};

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_NODE_ANALYZER_H 