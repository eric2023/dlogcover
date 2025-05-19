/**
 * @file ast_function_analyzer.cpp
 * @brief AST函数分析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_function_analyzer.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

ASTFunctionAnalyzer::ASTFunctionAnalyzer(clang::ASTContext& context, const std::string& filePath,
                                         const config::Config& config)
    : ASTNodeAnalyzer(context, filePath), stmtAnalyzer_(context, filePath), exprAnalyzer_(context, filePath, config) {}

Result<std::unique_ptr<ASTNodeInfo>> ASTFunctionAnalyzer::analyzeFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl || !decl->hasBody()) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INVALID_AST, "无效的函数声明或函数体");
    }

    LOG_DEBUG_FMT("分析函数声明: %s", decl->getNameAsString().c_str());

    // 创建函数节点
    auto nodeInfo = createNodeInfo(NodeType::FUNCTION, decl->getNameAsString(), decl->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    // 获取函数文本
    nodeInfo->text = getSourceText(decl->getBeginLoc(), decl->getEndLoc());

    // 分析函数体
    auto result = analyzeFunctionBody(decl->getBody());
    if (!result.hasError()) {
        auto& bodyNode = result.value();
        if (bodyNode && bodyNode->hasLogging) {
            nodeInfo->hasLogging = true;
        }
        if (bodyNode) {
            nodeInfo->children.push_back(std::move(bodyNode));
        }
    }

    LOG_DEBUG_FMT("函数 %s 分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->name.c_str(), nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTFunctionAnalyzer::analyzeMethodDecl(clang::CXXMethodDecl* decl) {
    if (!decl || !decl->hasBody()) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INVALID_AST, "无效的方法声明或方法体");
    }

    LOG_DEBUG_FMT("分析方法声明: %s::%s", decl->getParent()->getNameAsString().c_str(),
                  decl->getNameAsString().c_str());

    // 创建方法节点
    std::string methodName = decl->getParent()->getNameAsString() + "::" + decl->getNameAsString();
    auto nodeInfo = createNodeInfo(NodeType::METHOD, methodName, decl->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    // 获取方法文本
    nodeInfo->text = getSourceText(decl->getBeginLoc(), decl->getEndLoc());

    // 分析方法体
    auto result = analyzeFunctionBody(decl->getBody());
    if (!result.hasError()) {
        auto& bodyNode = result.value();
        if (bodyNode && bodyNode->hasLogging) {
            nodeInfo->hasLogging = true;
        }
        if (bodyNode) {
            nodeInfo->children.push_back(std::move(bodyNode));
        }
    }

    LOG_DEBUG_FMT("方法 %s 分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->name.c_str(), nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTFunctionAnalyzer::analyzeFunctionBody(clang::Stmt* body) {
    if (!body) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INVALID_AST, "无效的函数体");
    }

    LOG_DEBUG("分析函数体");

    // 使用语句分析器分析函数体
    return stmtAnalyzer_.analyzeStmt(body);
}

}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
