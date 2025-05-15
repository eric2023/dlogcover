/**
 * @file ast_analyzer.cpp
 * @brief AST分析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/utils/log_utils.h>

// 使用Clang/LLVM头文件
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <sstream>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

ASTAnalyzer::ASTAnalyzer(const config::Config& config, const source_manager::SourceManager& sourceManager)
    : config_(config), sourceManager_(sourceManager) {
    LOG_DEBUG("AST分析器初始化");
}

ASTAnalyzer::~ASTAnalyzer() {
    LOG_DEBUG("AST分析器销毁");
}

bool ASTAnalyzer::analyze(const std::string& filePath) {
    LOG_INFO_FMT("分析文件: %s", filePath.c_str());

    // 获取源文件信息
    const source_manager::SourceFileInfo* sourceFile = sourceManager_.getSourceFile(filePath);
    if (!sourceFile) {
        LOG_ERROR_FMT("无法找到源文件: %s", filePath.c_str());
        return false;
    }

    // 读取文件内容
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR_FMT("无法打开文件: %s", filePath.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // 创建AST单元
    auto astUnit = createASTUnit(filePath, content);
    if (!astUnit) {
        LOG_ERROR_FMT("创建AST单元失败: %s", filePath.c_str());
        return false;
    }

    // 分析AST上下文
    auto rootNode = analyzeASTContext(astUnit->getASTContext(), filePath);
    if (!rootNode) {
        LOG_ERROR_FMT("分析AST上下文失败: %s", filePath.c_str());
        return false;
    }

    // 保存AST节点信息
    astNodes_[filePath] = std::move(rootNode);

    LOG_INFO_FMT("文件分析完成: %s", filePath.c_str());
    return true;
}

bool ASTAnalyzer::analyzeAll() {
    LOG_INFO("开始分析所有源文件");

    bool success = true;
    const auto& sourceFiles = sourceManager_.getSourceFiles();

    for (const auto& sourceFile : sourceFiles) {
        if (!analyze(sourceFile.path)) {
            LOG_ERROR_FMT("分析文件失败: %s", sourceFile.path.c_str());
            success = false;
        }
    }

    LOG_INFO_FMT("所有文件分析完成，成功率: %s", success ? "100%" : "部分失败");
    return success;
}

const ASTNodeInfo* ASTAnalyzer::getASTNodeInfo(const std::string& filePath) const {
    auto it = astNodes_.find(filePath);
    if (it != astNodes_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<ASTNodeInfo>>& ASTAnalyzer::getAllASTNodeInfo() const {
    return astNodes_;
}

// 使用Clang/LLVM API创建AST单元
std::unique_ptr<clang::ASTUnit> ASTAnalyzer::createASTUnit(const std::string& filePath, const std::string& content) {
    LOG_DEBUG_FMT("创建文件的AST单元: %s", filePath.c_str());

    // 准备命令行参数，指定C++语言和标准
    std::vector<std::string> args = {"-std=c++17", "-xc++", "-Wall", "-Wextra", "-I" + config_.scan.includePathsStr};

    // 为Qt项目添加常用的定义
    if (config_.scan.isQtProject) {
        args.push_back("-DQT_CORE_LIB");
        args.push_back("-DQT_GUI_LIB");
        args.push_back("-DQT_WIDGETS_LIB");
    }

    // 添加用户自定义的编译参数
    for (const auto& arg : config_.scan.compilerArgs) {
        args.push_back(arg);
    }

    // 使用Clang的工具链创建AST
    std::unique_ptr<clang::ASTUnit> astUnit = clang::tooling::buildASTFromCodeWithArgs(content, args, filePath);

    if (!astUnit) {
        LOG_ERROR_FMT("无法为文件创建AST单元: %s", filePath.c_str());
        return nullptr;
    }

    LOG_DEBUG_FMT("成功创建AST单元: %s", filePath.c_str());
    return astUnit;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeASTContext(clang::ASTContext& context, const std::string& filePath) {
    LOG_DEBUG_FMT("分析AST上下文: %s", filePath.c_str());

    // 创建根节点
    auto rootNode = std::make_unique<ASTNodeInfo>();
    rootNode->type = NodeType::FUNCTION;  // 根节点类型设为FUNCTION只是作为容器
    rootNode->name = "root";
    rootNode->location.filePath = filePath;
    rootNode->location.line = 1;
    rootNode->location.column = 1;

    // 遍历顶层声明
    for (auto* decl : context.getTranslationUnitDecl()->decls()) {
        // 分析是否是函数声明
        if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            // 检查函数是否在当前源文件中定义
            if (funcDecl->hasBody() && context.getSourceManager().isInMainFile(funcDecl->getBeginLoc())) {
                auto funcNode = analyzeFunctionDecl(funcDecl, filePath);
                if (funcNode) {
                    rootNode->children.push_back(std::move(funcNode));
                }
            }
        }
        // 分析是否是类定义
        else if (auto* classDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
            // 检查类是否在当前源文件中定义
            if (classDecl->hasDefinition() && context.getSourceManager().isInMainFile(classDecl->getBeginLoc())) {
                // 遍历类中的方法
                for (auto* method : classDecl->methods()) {
                    if (method->hasBody()) {
                        auto methodNode = analyzeMethodDecl(method, filePath);
                        if (methodNode) {
                            rootNode->children.push_back(std::move(methodNode));
                        }
                    }
                }
            }
        }
    }

    // 检查根节点子节点中是否有日志调用，如果有，则设置hasLogging为true
    for (const auto& child : rootNode->children) {
        if (child->hasLogging) {
            rootNode->hasLogging = true;
            break;
        }
    }

    LOG_INFO_FMT("AST上下文分析完成，找到 %zu 个顶层函数/方法", rootNode->children.size());
    return rootNode;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeFunctionDecl(clang::FunctionDecl* decl, const std::string& filePath) {
    if (!decl || !decl->hasBody()) {
        return nullptr;
    }

    LOG_DEBUG_FMT("分析函数声明: %s", decl->getNameAsString().c_str());

    // 获取函数位置信息
    clang::SourceLocation loc = decl->getBeginLoc();
    clang::SourceManager& SM = decl->getASTContext().getSourceManager();
    unsigned line = SM.getSpellingLineNumber(loc);
    unsigned column = SM.getSpellingColumnNumber(loc);

    // 创建函数节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::FUNCTION;
    nodeInfo->name = decl->getNameAsString();
    nodeInfo->location.filePath = filePath;
    nodeInfo->location.line = line;
    nodeInfo->location.column = column;

    // 获取函数文本表示
    clang::SourceLocation endLoc = decl->getEndLoc();
    std::string text;
    if (loc.isValid() && endLoc.isValid()) {
        unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
        llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
        if (SM.getFileOffset(loc) + length <= fileData.size()) {
            text = fileData.substr(SM.getFileOffset(loc), length).str();
            // 限制文本长度
            if (text.length() > 200) {
                text = text.substr(0, 197) + "...";
            }
        }
    }
    nodeInfo->text = text;

    // 分析函数体
    if (auto* body = decl->getBody()) {
        if (auto* compoundStmt = llvm::dyn_cast<clang::CompoundStmt>(body)) {
            for (auto* stmt : compoundStmt->body()) {
                auto stmtNode = analyzeStmt(stmt, filePath);
                if (stmtNode) {
                    // 检查子节点是否包含日志调用
                    if (stmtNode->hasLogging) {
                        nodeInfo->hasLogging = true;
                    }
                    nodeInfo->children.push_back(std::move(stmtNode));
                }
            }
        }
    }

    LOG_DEBUG_FMT("函数 %s 分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->name.c_str(), nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeMethodDecl(clang::CXXMethodDecl* decl, const std::string& filePath) {
    if (!decl || !decl->hasBody()) {
        return nullptr;
    }

    LOG_DEBUG_FMT("分析方法声明: %s::%s", decl->getParent()->getNameAsString().c_str(),
                  decl->getNameAsString().c_str());

    // 获取方法位置信息
    clang::SourceLocation loc = decl->getBeginLoc();
    clang::SourceManager& SM = decl->getASTContext().getSourceManager();
    unsigned line = SM.getSpellingLineNumber(loc);
    unsigned column = SM.getSpellingColumnNumber(loc);

    // 创建方法节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::METHOD;
    nodeInfo->name = decl->getParent()->getNameAsString() + "::" + decl->getNameAsString();
    nodeInfo->location.filePath = filePath;
    nodeInfo->location.line = line;
    nodeInfo->location.column = column;

    // 获取方法文本表示
    clang::SourceLocation endLoc = decl->getEndLoc();
    std::string text;
    if (loc.isValid() && endLoc.isValid()) {
        unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
        llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
        if (SM.getFileOffset(loc) + length <= fileData.size()) {
            text = fileData.substr(SM.getFileOffset(loc), length).str();
            // 限制文本长度
            if (text.length() > 200) {
                text = text.substr(0, 197) + "...";
            }
        }
    }
    nodeInfo->text = text;

    // 分析方法体
    if (auto* body = decl->getBody()) {
        if (auto* compoundStmt = llvm::dyn_cast<clang::CompoundStmt>(body)) {
            for (auto* stmt : compoundStmt->body()) {
                auto stmtNode = analyzeStmt(stmt, filePath);
                if (stmtNode) {
                    // 检查子节点是否包含日志调用
                    if (stmtNode->hasLogging) {
                        nodeInfo->hasLogging = true;
                    }
                    nodeInfo->children.push_back(std::move(stmtNode));
                }
            }
        }
    }

    LOG_DEBUG_FMT("方法 %s 分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->name.c_str(), nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeStmt(clang::Stmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG_FMT("分析语句，类型ID: %d", stmt->getStmtClass());

    // 根据语句类型调用不同的分析方法
    if (auto* compoundStmt = llvm::dyn_cast<clang::CompoundStmt>(stmt)) {
        return analyzeCompoundStmt(compoundStmt, filePath);
    } else if (auto* ifStmt = llvm::dyn_cast<clang::IfStmt>(stmt)) {
        return analyzeIfStmt(ifStmt, filePath);
    } else if (auto* switchStmt = llvm::dyn_cast<clang::SwitchStmt>(stmt)) {
        return analyzeSwitchStmt(switchStmt, filePath);
    } else if (auto* forStmt = llvm::dyn_cast<clang::ForStmt>(stmt)) {
        return analyzeForStmt(forStmt, filePath);
    } else if (auto* whileStmt = llvm::dyn_cast<clang::WhileStmt>(stmt)) {
        return analyzeWhileStmt(whileStmt, filePath);
    } else if (auto* doStmt = llvm::dyn_cast<clang::DoStmt>(stmt)) {
        return analyzeDoStmt(doStmt, filePath);
    } else if (auto* tryStmt = llvm::dyn_cast<clang::CXXTryStmt>(stmt)) {
        return analyzeTryStmt(tryStmt, filePath);
    } else if (auto* catchStmt = llvm::dyn_cast<clang::CXXCatchStmt>(stmt)) {
        return analyzeCatchStmt(catchStmt, filePath);
    } else if (auto* callExpr = llvm::dyn_cast<clang::CallExpr>(stmt)) {
        return analyzeCallExpr(callExpr, filePath);
    } else {
        // 对于其他类型的语句，创建一个通用节点
        auto nodeInfo = std::make_unique<ASTNodeInfo>();
        nodeInfo->type = NodeType::CALL_EXPR;  // 默认为一般表达式

        // 获取语句位置信息
        clang::SourceLocation loc = stmt->getBeginLoc();
        if (loc.isValid()) {
            clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
            nodeInfo->location.filePath = filePath;
            nodeInfo->location.line = SM.getSpellingLineNumber(loc);
            nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

            // 获取语句文本表示
            clang::SourceLocation endLoc = stmt->getEndLoc();
            if (endLoc.isValid()) {
                unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
                llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
                if (SM.getFileOffset(loc) + length <= fileData.size()) {
                    nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                    // 限制文本长度
                    if (nodeInfo->text.length() > 100) {
                        nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                    }
                }
            }
        }

        return nodeInfo;
    }
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeCompoundStmt(clang::CompoundStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析复合语句");

    // 创建复合语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::CALL_EXPR;  // 使用通用类型，因为我们没有专门的复合语句类型

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示 (只获取开始的花括号)
        nodeInfo->text = "{...}";
    }

    // 分析复合语句中的每条语句
    for (auto* subStmt : stmt->body()) {
        auto subNode = analyzeStmt(subStmt, filePath);
        if (subNode) {
            // 检查子节点是否包含日志调用
            if (subNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(subNode));
        }
    }

    LOG_DEBUG_FMT("复合语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeIfStmt(clang::IfStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析if语句");

    // 创建if语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::IF_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = "if (...) {...}";
        }
    }

    // 分析if语句的then部分
    if (stmt->getThen()) {
        auto thenNode = analyzeStmt(stmt->getThen(), filePath);
        if (thenNode) {
            // 检查子节点是否包含日志调用
            if (thenNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(thenNode));
        }
    }

    // 分析if语句的else部分
    if (stmt->getElse()) {
        // 为else部分创建一个独立的节点
        auto elseNode = std::make_unique<ASTNodeInfo>();
        elseNode->type = NodeType::ELSE_STMT;

        // 获取else部分的位置信息
        clang::SourceLocation elseLoc = stmt->getElseLoc();
        if (elseLoc.isValid()) {
            clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
            elseNode->location.filePath = filePath;
            elseNode->location.line = SM.getSpellingLineNumber(elseLoc);
            elseNode->location.column = SM.getSpellingColumnNumber(elseLoc);
            elseNode->text = "else {...}";
        }

        // 分析else部分的语句
        auto elseStmtNode = analyzeStmt(stmt->getElse(), filePath);
        if (elseStmtNode) {
            // 检查else语句是否包含日志调用
            if (elseStmtNode->hasLogging) {
                elseNode->hasLogging = true;
                // 如果else部分有日志，那么整个if语句都被标记为有日志
                nodeInfo->hasLogging = true;
            }
            elseNode->children.push_back(std::move(elseStmtNode));
        }

        // 将else节点添加到if节点的子节点中
        nodeInfo->children.push_back(std::move(elseNode));
    }

    LOG_DEBUG_FMT("if语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeSwitchStmt(clang::SwitchStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析switch语句");

    // 创建switch语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::SWITCH_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = "switch (...) {...}";
        }
    }

    // 分析switch语句体
    if (auto* body = stmt->getBody()) {
        auto bodyNode = analyzeStmt(body, filePath);
        if (bodyNode) {
            // 检查子节点是否包含日志调用
            if (bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(bodyNode));
        }

        // 注意：case语句将在analyzeStmt中处理，因为它们是CompoundStmt的一部分
    }

    LOG_DEBUG_FMT("switch语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeForStmt(clang::ForStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析for循环语句");

    // 创建for循环语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::FOR_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = "for (...) {...}";
        }
    }

    // 分析for循环语句体
    if (stmt->getBody()) {
        auto bodyNode = analyzeStmt(stmt->getBody(), filePath);
        if (bodyNode) {
            // 检查子节点是否包含日志调用
            if (bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(bodyNode));
        }
    }

    LOG_DEBUG_FMT("for循环语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeWhileStmt(clang::WhileStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析while循环语句");

    // 创建while循环语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::WHILE_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = "while (...) {...}";
        }
    }

    // 分析while循环语句体
    if (stmt->getBody()) {
        auto bodyNode = analyzeStmt(stmt->getBody(), filePath);
        if (bodyNode) {
            // 检查子节点是否包含日志调用
            if (bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(bodyNode));
        }
    }

    LOG_DEBUG_FMT("while循环语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeDoStmt(clang::DoStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析do-while循环语句");

    // 创建do-while循环语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::DO_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = "do {...} while (...);";
        }
    }

    // 分析do-while循环语句体
    if (stmt->getBody()) {
        auto bodyNode = analyzeStmt(stmt->getBody(), filePath);
        if (bodyNode) {
            // 检查子节点是否包含日志调用
            if (bodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(bodyNode));
        }
    }

    LOG_DEBUG_FMT("do-while循环语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeTryStmt(clang::CXXTryStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析try语句");

    // 创建try语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::TRY_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = "try {...} catch (...) {...}";
        }
    }

    // 分析try语句体
    if (stmt->getTryBlock()) {
        auto tryBodyNode = analyzeStmt(stmt->getTryBlock(), filePath);
        if (tryBodyNode) {
            // 检查子节点是否包含日志调用
            if (tryBodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(tryBodyNode));
        }
    }

    // 分析所有catch语句
    for (unsigned i = 0; i < stmt->getNumHandlers(); ++i) {
        auto* handler = stmt->getHandler(i);
        auto catchNode = analyzeCatchStmt(handler, filePath);
        if (catchNode) {
            // 检查子节点是否包含日志调用
            if (catchNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(catchNode));
        }
    }

    LOG_DEBUG_FMT("try语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeCatchStmt(clang::CXXCatchStmt* stmt, const std::string& filePath) {
    if (!stmt) {
        return nullptr;
    }

    LOG_DEBUG("分析catch语句");

    // 创建catch语句节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = NodeType::CATCH_STMT;

    // 获取语句位置信息
    clang::SourceLocation loc = stmt->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = stmt->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取语句文本表示
        clang::SourceLocation endLoc = stmt->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            // 获取异常类型
            std::string exceptionType = "...";
            if (stmt->getExceptionDecl()) {
                if (auto* typeInfo = stmt->getExceptionDecl()->getType().getTypePtr()) {
                    llvm::raw_string_ostream typeName(exceptionType);
                    typeInfo->print(typeName, stmt->getASTContext().getPrintingPolicy());
                }
            }
            nodeInfo->text = "catch (" + exceptionType + ") {...}";
        }
    }

    // 分析catch语句体
    if (stmt->getHandlerBlock()) {
        auto catchBodyNode = analyzeStmt(stmt->getHandlerBlock(), filePath);
        if (catchBodyNode) {
            // 检查子节点是否包含日志调用
            if (catchBodyNode->hasLogging) {
                nodeInfo->hasLogging = true;
            }
            nodeInfo->children.push_back(std::move(catchBodyNode));
        }
    }

    LOG_DEBUG_FMT("catch语句分析完成，有 %zu 个子节点，%s日志调用", nodeInfo->children.size(),
                  nodeInfo->hasLogging ? "包含" : "不包含");

    return nodeInfo;
}

std::unique_ptr<ASTNodeInfo> ASTAnalyzer::analyzeCallExpr(clang::CallExpr* expr, const std::string& filePath) {
    if (!expr) {
        return nullptr;
    }

    // 获取被调用的函数名称
    std::string funcName;
    if (auto* callee = expr->getCalleeDecl()) {
        if (auto* funcDecl = llvm::dyn_cast<clang::NamedDecl>(callee)) {
            funcName = funcDecl->getNameAsString();
        }
    } else if (auto* callee = expr->getCallee()) {
        if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(callee->IgnoreParenCasts())) {
            if (auto* funcDecl = llvm::dyn_cast<clang::NamedDecl>(declRef->getDecl())) {
                funcName = funcDecl->getNameAsString();
            }
        } else if (auto* memberExpr = llvm::dyn_cast<clang::MemberExpr>(callee->IgnoreParenCasts())) {
            if (auto* funcDecl = llvm::dyn_cast<clang::NamedDecl>(memberExpr->getMemberDecl())) {
                funcName = funcDecl->getNameAsString();
            }
        }
    }

    LOG_DEBUG_FMT("分析函数调用: %s", funcName.c_str());

    // 检查是否是日志函数调用
    bool isLog = isLogFunctionCall(expr);

    // 创建函数调用节点
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = isLog ? NodeType::LOG_CALL_EXPR : NodeType::CALL_EXPR;
    nodeInfo->name = funcName;
    nodeInfo->hasLogging = isLog;

    // 获取函数调用位置信息
    clang::SourceLocation loc = expr->getBeginLoc();
    if (loc.isValid()) {
        clang::SourceManager& SM = expr->getASTContext().getSourceManager();
        nodeInfo->location.filePath = filePath;
        nodeInfo->location.line = SM.getSpellingLineNumber(loc);
        nodeInfo->location.column = SM.getSpellingColumnNumber(loc);

        // 获取函数调用文本表示
        clang::SourceLocation endLoc = expr->getEndLoc();
        if (endLoc.isValid()) {
            unsigned length = SM.getFileOffset(endLoc) - SM.getFileOffset(loc);
            llvm::StringRef fileData = SM.getBufferData(SM.getFileID(loc));
            if (SM.getFileOffset(loc) + length <= fileData.size()) {
                nodeInfo->text = fileData.substr(SM.getFileOffset(loc), length).str();
                // 限制文本长度
                if (nodeInfo->text.length() > 100) {
                    nodeInfo->text = nodeInfo->text.substr(0, 97) + "...";
                }
            }
        } else {
            nodeInfo->text = funcName + "(...)";
        }
    }

    // 分析函数调用参数
    // 这部分我们简化处理，不递归分析参数中的表达式

    if (isLog) {
        LOG_DEBUG_FMT("识别到日志函数调用: %s", funcName.c_str());
    }

    return nodeInfo;
}

bool ASTAnalyzer::isLogFunctionCall(clang::CallExpr* expr) const {
    if (!expr) {
        return false;
    }

    // 获取被调用的函数名称
    std::string funcName;
    if (auto* callee = expr->getCalleeDecl()) {
        if (auto* funcDecl = llvm::dyn_cast<clang::NamedDecl>(callee)) {
            funcName = funcDecl->getNameAsString();
        }
    } else if (auto* callee = expr->getCallee()) {
        if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(callee->IgnoreParenCasts())) {
            if (auto* funcDecl = llvm::dyn_cast<clang::NamedDecl>(declRef->getDecl())) {
                funcName = funcDecl->getNameAsString();
            }
        } else if (auto* memberExpr = llvm::dyn_cast<clang::MemberExpr>(callee->IgnoreParenCasts())) {
            if (auto* funcDecl = llvm::dyn_cast<clang::NamedDecl>(memberExpr->getMemberDecl())) {
                funcName = funcDecl->getNameAsString();
            }
        }
    }

    // 检查是否为Qt日志函数
    if (config_.logFunctions.qt.enabled) {
        for (const auto& logFunc : config_.logFunctions.qt.functions) {
            if (funcName == logFunc) {
                return true;
            }
        }
        for (const auto& logFunc : config_.logFunctions.qt.categoryFunctions) {
            if (funcName == logFunc) {
                return true;
            }
        }
    }

    // 检查是否为自定义日志函数
    if (config_.logFunctions.custom.enabled) {
        for (const auto& [_, funcs] : config_.logFunctions.custom.functions) {
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
