/**
 * @file ast_statement_analyzer.cpp
 * @brief AST语句分析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_statement_analyzer.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeStmt(clang::Stmt* stmt) {
    if (!stmt) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INVALID_AST, "无效的语句");
    }

    LOG_DEBUG_FMT("分析语句，类型ID: %d", stmt->getStmtClass());

    // 根据语句类型调用不同的分析方法
    if (auto* compoundStmt = llvm::dyn_cast<clang::CompoundStmt>(stmt)) {
        return analyzeCompoundStmt(compoundStmt);
    } else if (auto* ifStmt = llvm::dyn_cast<clang::IfStmt>(stmt)) {
        return analyzeIfStmt(ifStmt);
    } else if (auto* switchStmt = llvm::dyn_cast<clang::SwitchStmt>(stmt)) {
        return analyzeSwitchStmt(switchStmt);
    } else if (auto* forStmt = llvm::dyn_cast<clang::ForStmt>(stmt)) {
        return analyzeForStmt(forStmt);
    } else if (auto* whileStmt = llvm::dyn_cast<clang::WhileStmt>(stmt)) {
        return analyzeWhileStmt(whileStmt);
    } else if (auto* doStmt = llvm::dyn_cast<clang::DoStmt>(stmt)) {
        return analyzeDoStmt(doStmt);
    } else if (auto* tryStmt = llvm::dyn_cast<clang::CXXTryStmt>(stmt)) {
        return analyzeTryStmt(tryStmt);
    } else if (auto* catchStmt = llvm::dyn_cast<clang::CXXCatchStmt>(stmt)) {
        return analyzeCatchStmt(catchStmt);
    }

    // 对于其他类型的语句，创建一个通用节点
    auto nodeInfo = createNodeInfo(NodeType::CALL_EXPR, "statement", stmt->getBeginLoc());
    if (nodeInfo) {
        nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());
    }
    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeCompoundStmt(clang::CompoundStmt* stmt) {
    if (!stmt) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INVALID_AST, "复合语句为空");
    }

    auto nodeInfo = createNodeInfo(NodeType::FUNCTION, "compound", stmt->getBeginLoc());

    for (auto* subStmt : stmt->body()) {
        auto result = analyzeStmt(subStmt);
        if (result.hasError()) {
            LOG_WARNING_FMT("分析子语句失败: %s", result.errorMessage().c_str());
            continue;
        }
        nodeInfo->children.push_back(std::move(result.value()));
    }

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeIfStmt(clang::IfStmt* stmt) {
    LOG_DEBUG("分析if语句");

    auto nodeInfo = createNodeInfo(NodeType::IF_STMT, "if", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析if语句的then部分
    if (stmt->getThen()) {
        auto result = analyzeStmt(stmt->getThen());
        if (!result.hasError()) {
            auto& thenNode = result.value();
            if (thenNode && thenNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (thenNode) {
                nodeInfo->children.push_back(std::move(thenNode));
            }
        }
    }

    // 分析if语句的else部分
    if (stmt->getElse()) {
        auto elseNode = createNodeInfo(NodeType::ELSE_STMT, "else", stmt->getElseLoc(), "else {...}");
        if (elseNode) {
            auto result = analyzeStmt(stmt->getElse());
            if (!result.hasError()) {
                auto& elseStmtNode = result.value();
                if (elseStmtNode && elseStmtNode->hasLogging) {
                    elseNode->hasLogging = true;
                    nodeInfo->hasLogging = true;
                }
                if (elseStmtNode) {
                    elseNode->children.push_back(std::move(elseStmtNode));
                }
            }
            nodeInfo->children.push_back(std::move(elseNode));
        }
    }

    LOG_DEBUG_FMT("if语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeSwitchStmt(clang::SwitchStmt* stmt) {
    LOG_DEBUG("分析switch语句");

    auto nodeInfo = createNodeInfo(NodeType::SWITCH_STMT, "switch", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析switch语句的body
    if (stmt->getBody()) {
        auto result = analyzeStmt(stmt->getBody());
        if (!result.hasError()) {
            auto& bodyNode = result.value();
            if (bodyNode && bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (bodyNode) {
                nodeInfo->children.push_back(std::move(bodyNode));
            }
        }
    }

    LOG_DEBUG_FMT("switch语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeForStmt(clang::ForStmt* stmt) {
    LOG_DEBUG("分析for语句");

    auto nodeInfo = createNodeInfo(NodeType::FOR_STMT, "for", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析for语句的body
    if (stmt->getBody()) {
        auto result = analyzeStmt(stmt->getBody());
        if (!result.hasError()) {
            auto& bodyNode = result.value();
            if (bodyNode && bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (bodyNode) {
                nodeInfo->children.push_back(std::move(bodyNode));
            }
        }
    }

    LOG_DEBUG_FMT("for语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeWhileStmt(clang::WhileStmt* stmt) {
    LOG_DEBUG("分析while语句");

    auto nodeInfo = createNodeInfo(NodeType::WHILE_STMT, "while", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析while语句的body
    if (stmt->getBody()) {
        auto result = analyzeStmt(stmt->getBody());
        if (!result.hasError()) {
            auto& bodyNode = result.value();
            if (bodyNode && bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (bodyNode) {
                nodeInfo->children.push_back(std::move(bodyNode));
            }
        }
    }

    LOG_DEBUG_FMT("while语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeDoStmt(clang::DoStmt* stmt) {
    LOG_DEBUG("分析do语句");

    auto nodeInfo = createNodeInfo(NodeType::DO_STMT, "do", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析do语句的body
    if (stmt->getBody()) {
        auto result = analyzeStmt(stmt->getBody());
        if (!result.hasError()) {
            auto& bodyNode = result.value();
            if (bodyNode && bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (bodyNode) {
                nodeInfo->children.push_back(std::move(bodyNode));
            }
        }
    }

    LOG_DEBUG_FMT("do语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeTryStmt(clang::CXXTryStmt* stmt) {
    LOG_DEBUG("分析try语句");

    auto nodeInfo = createNodeInfo(NodeType::TRY_STMT, "try", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析try语句的try块
    if (stmt->getTryBlock()) {
        auto result = analyzeStmt(stmt->getTryBlock());
        if (!result.hasError()) {
            auto& tryNode = result.value();
            if (tryNode && tryNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (tryNode) {
                nodeInfo->children.push_back(std::move(tryNode));
            }
        }
    }

    // 分析try语句的catch块
    for (unsigned i = 0; i < stmt->getNumHandlers(); ++i) {
        auto* handler = stmt->getHandler(i);
        auto result = analyzeCatchStmt(handler);
        if (!result.hasError()) {
            auto& catchNode = result.value();
            if (catchNode && catchNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (catchNode) {
                nodeInfo->children.push_back(std::move(catchNode));
            }
        }
    }

    LOG_DEBUG_FMT("try语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

Result<std::unique_ptr<ASTNodeInfo>> ASTStatementAnalyzer::analyzeCatchStmt(clang::CXXCatchStmt* stmt) {
    LOG_DEBUG("分析catch语句");

    auto nodeInfo = createNodeInfo(NodeType::CATCH_STMT, "catch", stmt->getBeginLoc());
    if (!nodeInfo) {
        return makeError<std::unique_ptr<ASTNodeInfo>>(ASTAnalyzerError::INTERNAL_ERROR, "创建节点信息失败");
    }

    nodeInfo->text = getSourceText(stmt->getBeginLoc(), stmt->getEndLoc());

    // 分析catch语句的body
    if (stmt->getHandlerBlock()) {
        auto result = analyzeStmt(stmt->getHandlerBlock());
        if (!result.hasError()) {
            auto& bodyNode = result.value();
            if (bodyNode && bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            if (bodyNode) {
                nodeInfo->children.push_back(std::move(bodyNode));
            }
        }
    }

    LOG_DEBUG_FMT("catch语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return makeSuccess(std::move(nodeInfo));
}

}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
