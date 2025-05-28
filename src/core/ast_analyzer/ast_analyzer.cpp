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
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

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

Result<bool> ASTAnalyzer::analyze(const std::string& filePath) {
    LOG_INFO_FMT("分析文件: %s", filePath.c_str());

    // 获取源文件信息
    const source_manager::SourceFileInfo* sourceFile = sourceManager_.getSourceFile(filePath);
    if (!sourceFile) {
        return makeError<bool>(ASTAnalyzerError::FILE_NOT_FOUND, "无法找到源文件: " + filePath);
    }

    // 读取文件内容
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return makeError<bool>(ASTAnalyzerError::FILE_READ_ERROR, "无法打开文件: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // 创建AST单元
    currentASTUnit_ = createASTUnit(filePath, content);
    if (!currentASTUnit_) {
        return makeError<bool>(ASTAnalyzerError::COMPILATION_ERROR, "创建AST单元失败: " + filePath);
    }

    // 分析AST上下文
    LOG_INFO_FMT("准备调用analyzeASTContext: %s", filePath.c_str());
    auto result = analyzeASTContext(currentASTUnit_->getASTContext(), filePath);
    if (result.hasError()) {
        return makeError<bool>(result.error(), result.errorMessage());
    }

    // 保存AST节点信息
    astNodes_[filePath] = std::move(result.value());

    LOG_INFO_FMT("文件分析完成: %s", filePath.c_str());
    return makeSuccess(true);
}

Result<bool> ASTAnalyzer::analyzeAll() {
    LOG_DEBUG("开始分析所有源文件");

    bool allSuccess = true;
    for (const auto& sourceFile : sourceManager_.getSourceFiles()) {
        auto result = analyze(sourceFile.path);
        if (result.hasError()) {
            LOG_ERROR_FMT("分析文件失败: %s, 错误: %s", sourceFile.path.c_str(), result.errorMessage().c_str());
            allSuccess = false;
        }
    }

    return makeSuccess(std::move(allSuccess));
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

    // 创建编译命令行参数
    std::vector<std::string> args;

    // 基本命令行选项
    args.push_back("-std=c++17");
    args.push_back("-xc++");
    args.push_back("-Wall");
    args.push_back("-Wextra");
    args.push_back("-fPIC");
    args.push_back("-Wno-everything");  // 忽略所有警告
    args.push_back("-ferror-limit=0");  // 不限制错误数量
    args.push_back("-fsyntax-only");    // 仅进行语法检查

    // 添加系统头文件路径
    args.push_back("-I/usr/include");
    args.push_back("-I/usr/include/c++/11");
    args.push_back("-I/usr/include/x86_64-linux-gnu/c++/11");
    args.push_back("-I/usr/include/c++/11/backward");
    args.push_back("-I/usr/lib/gcc/x86_64-linux-gnu/11/include");
    args.push_back("-I/usr/local/include");
    args.push_back("-I/usr/lib/gcc/x86_64-linux-gnu/11/include-fixed");
    args.push_back("-I/usr/include/x86_64-linux-gnu");

    args.push_back("-I/usr/lib/llvm-17/include");

    // 添加包含路径
    if (!config_.scan.includePathsStr.empty()) {
        std::istringstream iss(config_.scan.includePathsStr);
        std::string includePath;
        while (std::getline(iss, includePath, ':')) {
            if (!includePath.empty()) {
                args.push_back("-I" + includePath);
            }
        }
    }

    // 默认添加当前目录和include目录
    args.push_back("-I.");
    args.push_back("-I./include");

    // 为Qt项目添加常用的定义和选项
    if (config_.scan.isQtProject) {
        args.push_back("-DQT_CORE_LIB");
        args.push_back("-DQT_GUI_LIB");
        args.push_back("-DQT_WIDGETS_LIB");
        args.push_back("-DQT_SHARED");
        args.push_back("-DQ_CREATOR_RUN");
        args.push_back("-D_REENTRANT");
    }

    // 添加用户自定义的编译参数
    for (const auto& arg : config_.scan.compilerArgs) {
        args.push_back(arg);
    }

    LOG_DEBUG_FMT("编译命令: %s", llvm::join(args, " ").c_str());

    // 使用Clang的工具链创建AST
    std::unique_ptr<clang::ASTUnit> astUnit = clang::tooling::buildASTFromCodeWithArgs(content, args, filePath);

    if (!astUnit) {
        LOG_ERROR_FMT("无法为文件创建AST单元: %s", filePath.c_str());
        return nullptr;
    }

    LOG_DEBUG_FMT("成功创建AST单元: %s", filePath.c_str());
    return astUnit;
}

Result<std::unique_ptr<ASTNodeInfo>> ASTAnalyzer::analyzeASTContext(clang::ASTContext& context,
                                                                    const std::string& filePath) {
    LOG_INFO_FMT("分析AST上下文: %s", filePath.c_str());

    // 创建函数分析器
    ASTFunctionAnalyzer functionAnalyzer(context, filePath, config_);
    // 创建根节点
    auto rootNode = std::make_unique<ASTNodeInfo>();
    rootNode->type = NodeType::UNKNOWN;  // 根节点类型设为FILE，表示文件级别的节点
    rootNode->name = std::filesystem::path(filePath).filename().string();
    rootNode->location.filePath = filePath;
    rootNode->location.line = 1;
    rootNode->location.column = 1;

    // 遍历顶层声明
    LOG_INFO_FMT("开始遍历翻译单元声明，文件: %s", filePath.c_str());
    for (auto* decl : context.getTranslationUnitDecl()->decls()) {
        // 分析是否是函数声明
        if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            // 检查函数是否在当前源文件中定义
            if (funcDecl->hasBody() && context.getSourceManager().isInMainFile(funcDecl->getBeginLoc())) {
                LOG_DEBUG_FMT("分析函数: %s", funcDecl->getNameAsString().c_str());
                auto result = functionAnalyzer.analyzeFunctionDecl(funcDecl);
                if (!result.hasError()) {
                    auto& funcNode = result.value();
                    if (funcNode) {
                        if (funcNode->hasLogging) {
                            rootNode->hasLogging = true;
                        }
                        rootNode->children.push_back(std::move(funcNode));
                    }
                } else {
                    LOG_WARNING_FMT("函数分析失败: %s, 错误: %s", 
                                   funcDecl->getNameAsString().c_str(), 
                                   result.errorMessage().c_str());
                }
            }
        }
        // 分析是否是类定义
        else if (auto* classDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
            // 检查类是否在当前源文件中定义
            if (classDecl->hasDefinition() && context.getSourceManager().isInMainFile(classDecl->getBeginLoc())) {
                LOG_DEBUG_FMT("分析类: %s", classDecl->getNameAsString().c_str());
                // 遍历类中的方法
                for (auto* method : classDecl->methods()) {
                    if (method->hasBody()) {
                        LOG_DEBUG_FMT("分析类方法: %s::%s", 
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                        auto result = functionAnalyzer.analyzeMethodDecl(method);
                        if (!result.hasError()) {
                            auto& methodNode = result.value();
                            if (methodNode) {
                                if (methodNode->hasLogging) {
                                    rootNode->hasLogging = true;
                                }
                                rootNode->children.push_back(std::move(methodNode));
                            }
                        } else {
                            LOG_WARNING_FMT("方法分析失败: %s::%s, 错误: %s", 
                                           classDecl->getNameAsString().c_str(),
                                           method->getNameAsString().c_str(),
                                           result.errorMessage().c_str());
                        }
                    }
                }
            }
        }
        // 分析是否是命名空间
        else if (auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
            // 检查命名空间是否在当前源文件中定义
            if (context.getSourceManager().isInMainFile(namespaceDecl->getBeginLoc())) {
                LOG_DEBUG_FMT("分析命名空间: %s", namespaceDecl->getNameAsString().c_str());
                // 递归遍历命名空间中的声明
                for (auto* innerDecl : namespaceDecl->decls()) {
                    if (auto* innerFuncDecl = llvm::dyn_cast<clang::FunctionDecl>(innerDecl)) {
                        if (innerFuncDecl->hasBody()) {
                            LOG_DEBUG_FMT("分析命名空间函数: %s::%s", 
                                         namespaceDecl->getNameAsString().c_str(),
                                         innerFuncDecl->getNameAsString().c_str());
                            auto result = functionAnalyzer.analyzeFunctionDecl(innerFuncDecl);
                            if (!result.hasError()) {
                                auto& funcNode = result.value();
                                if (funcNode) {
                                    if (funcNode->hasLogging) {
                                        rootNode->hasLogging = true;
                                    }
                                    rootNode->children.push_back(std::move(funcNode));
                                }
                            } else {
                                LOG_WARNING_FMT("命名空间函数分析失败: %s::%s, 错误: %s", 
                                               namespaceDecl->getNameAsString().c_str(),
                                               innerFuncDecl->getNameAsString().c_str(),
                                               result.errorMessage().c_str());
                            }
                        }
                    }
                }
            }
        }
    }

    LOG_INFO_FMT("AST上下文分析完成，找到 %zu 个顶层函数/方法", rootNode->children.size());
    return makeSuccess(std::move(rootNode));
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
