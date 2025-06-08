/**
 * @file ast_expression_analyzer.cpp
 * @brief AST表达式分析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_expression_analyzer.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

Result<std::unique_ptr<ASTNodeInfo>> ASTExpressionAnalyzer::analyzeExpr(clang::Expr* expr) {
    if (!expr) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INVALID_AST, "无效的表达式");
    }

    LOG_DEBUG_FMT("分析表达式，类型ID: %d", expr->getStmtClass());

    // 处理函数调用表达式
    if (auto* callExpr = llvm::dyn_cast<clang::CallExpr>(expr)) {
        return analyzeCallExpr(callExpr);
    }

    // 对于其他类型的表达式，创建一个通用节点
    auto nodeInfo = createNodeInfo(NodeType::CALL_EXPR, "expression", expr->getBeginLoc());
    if (nodeInfo) {
        nodeInfo->text = getSourceText(expr->getBeginLoc(), expr->getEndLoc());
    }
    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTExpressionAnalyzer::analyzeCallExpr(clang::CallExpr* expr) {
    LOG_DEBUG("分析函数调用表达式");

    std::string funcName = getFunctionName(expr);
    auto nodeInfo = createNodeInfo(NodeType::CALL_EXPR, funcName, expr->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(expr->getBeginLoc(), expr->getEndLoc());

    // 检查是否是日志函数调用
    if (isLogFunctionCall(expr)) {
        LOG_DEBUG_FMT("识别到日志函数调用: %s", funcName.c_str());
        nodeInfo->type = NodeType::LOG_CALL_EXPR;
        nodeInfo->hasLogging = true;
    }

    // 分析函数参数
    for (unsigned i = 0; i < expr->getNumArgs(); ++i) {
        if (auto* arg = expr->getArg(i)) {
            auto result = analyzeExpr(arg);
            if (!result.hasError()) {
                auto& argNode = result.value();
                if (argNode && argNode->hasLogging) {
                    nodeInfo->hasLogging = true;
                }
                if (argNode) {
                    nodeInfo->children.push_back(std::move(argNode));
                }
            }
        }
    }

    LOG_DEBUG_FMT("函数调用分析完成: %s, %s日志调用", nodeInfo->name.c_str(), nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

std::string ASTExpressionAnalyzer::getFunctionName(clang::CallExpr* expr) const {
    std::string funcName;

    if (const auto* callee = expr->getDirectCallee()) {
        funcName = callee->getNameAsString();
        LOG_DEBUG_FMT("直接调用函数名: %s", funcName.c_str());
    } else if (const auto* calleeExpr = expr->getCallee()) {
        if (const auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(calleeExpr->IgnoreParenImpCasts())) {
            funcName = declRef->getNameInfo().getAsString();
            LOG_DEBUG_FMT("通过DeclRefExpr获取函数名: %s", funcName.c_str());
        } else if (const auto* memberExpr = llvm::dyn_cast<clang::MemberExpr>(calleeExpr->IgnoreParenImpCasts())) {
            funcName = memberExpr->getMemberNameInfo().getAsString();
            LOG_DEBUG_FMT("通过MemberExpr获取函数名: %s", funcName.c_str());
        }
    }

    return funcName;
}

bool ASTExpressionAnalyzer::isLogFunctionCall(clang::CallExpr* expr) const {
    if (!expr) {
        return false;
    }

    std::string funcName = getFunctionName(expr);
    if (funcName.empty()) {
        return false;
    }

    // 检查是否为Qt日志函数
    if (config_.log_functions.qt.enabled) {
        for (const auto& logFunc : config_.log_functions.qt.functions) {
            if (funcName == logFunc) {
                return true;
            }
        }
        for (const auto& logFunc : config_.log_functions.qt.category_functions) {
            if (funcName == logFunc) {
                return true;
            }
        }
    }

    // 检查是否为自定义日志函数
    if (config_.log_functions.custom.enabled) {
        for (const auto& [_, funcs] : config_.log_functions.custom.functions) {
            for (const auto& logFunc : funcs) {
                if (funcName == logFunc) {
                    return true;
                }
            }
        }
    }

    return false;
}

}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
