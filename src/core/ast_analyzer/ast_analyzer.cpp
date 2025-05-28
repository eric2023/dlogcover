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
    args.push_back("-fPIC");
    
    // 调试和错误处理选项
    args.push_back("-Wno-everything");  // 忽略所有警告以避免干扰
    args.push_back("-ferror-limit=0");  // 不限制错误数量
    args.push_back("-fsyntax-only");    // 仅进行语法检查
    args.push_back("-fno-delayed-template-parsing");  // 立即解析模板
    
    // 系统头文件路径 - 更完整的路径列表
    std::vector<std::string> systemIncludes = {
        "/usr/include",
        "/usr/local/include",
        "/usr/include/c++/11",
        "/usr/include/c++/10",
        "/usr/include/c++/9",
        "/usr/include/x86_64-linux-gnu/c++/11",
        "/usr/include/x86_64-linux-gnu/c++/10", 
        "/usr/include/x86_64-linux-gnu/c++/9",
        "/usr/include/c++/11/backward",
        "/usr/include/c++/10/backward",
        "/usr/include/c++/9/backward",
        "/usr/lib/gcc/x86_64-linux-gnu/11/include",
        "/usr/lib/gcc/x86_64-linux-gnu/10/include",
        "/usr/lib/gcc/x86_64-linux-gnu/9/include",
        "/usr/lib/gcc/x86_64-linux-gnu/11/include-fixed",
        "/usr/lib/gcc/x86_64-linux-gnu/10/include-fixed",
        "/usr/lib/gcc/x86_64-linux-gnu/9/include-fixed",
        "/usr/include/x86_64-linux-gnu",
        "/usr/lib/llvm-17/include",
        "/usr/lib/llvm-16/include",
        "/usr/lib/llvm-15/include",
        "/usr/lib/llvm-14/include",
        "/usr/lib/llvm-13/include"
    };

    // 只添加存在的系统头文件路径
    for (const auto& includePath : systemIncludes) {
        if (std::filesystem::exists(includePath)) {
            args.push_back("-I" + includePath);
            LOG_DEBUG_FMT("添加系统头文件路径: %s", includePath.c_str());
        }
    }

    // 项目特定的包含路径
    std::vector<std::string> projectIncludes = {
        ".",          // 当前目录
        "./include",  // 项目include目录
        "./src",      // 源码目录
        "../include", // 上级include目录
        "../src"      // 上级源码目录
    };

    // 添加项目特定的包含路径
    for (const auto& includePath : projectIncludes) {
        args.push_back("-I" + includePath);
        LOG_DEBUG_FMT("添加项目头文件路径: %s", includePath.c_str());
    }

    // 添加配置文件中指定的包含路径
    if (!config_.scan.includePathsStr.empty()) {
        std::istringstream iss(config_.scan.includePathsStr);
        std::string includePath;
        while (std::getline(iss, includePath, ':')) {
            if (!includePath.empty()) {
                args.push_back("-I" + includePath);
                LOG_DEBUG_FMT("添加配置的头文件路径: %s", includePath.c_str());
            }
        }
    }

    // 基本系统定义
    args.push_back("-D__GNUG__");
    args.push_back("-D__linux__");
    args.push_back("-D__x86_64__");
    args.push_back("-D_GNU_SOURCE");
    args.push_back("-D__STDC_CONSTANT_MACROS");
    args.push_back("-D__STDC_FORMAT_MACROS");
    args.push_back("-D__STDC_LIMIT_MACROS");

    // 为Qt项目添加常用的定义和选项
    if (config_.scan.isQtProject) {
        args.push_back("-DQT_CORE_LIB");
        args.push_back("-DQT_GUI_LIB");
        args.push_back("-DQT_WIDGETS_LIB");
        args.push_back("-DQT_SHARED");
        args.push_back("-DQ_CREATOR_RUN");
        args.push_back("-D_REENTRANT");
        LOG_DEBUG("添加Qt项目相关定义");
    }

    // 添加用户自定义的编译参数
    for (const auto& arg : config_.scan.compilerArgs) {
        args.push_back(arg);
        LOG_DEBUG_FMT("添加用户自定义参数: %s", arg.c_str());
    }

    LOG_INFO_FMT("编译命令参数总数: %zu", args.size());
    LOG_DEBUG_FMT("完整编译命令: %s", llvm::join(args, " ").c_str());

    // 使用Clang的工具链创建AST
    std::unique_ptr<clang::ASTUnit> astUnit = clang::tooling::buildASTFromCodeWithArgs(content, args, filePath);

    if (!astUnit) {
        LOG_ERROR_FMT("无法为文件创建AST单元: %s", filePath.c_str());
        
        // 输出诊断信息
        LOG_ERROR("可能的原因:");
        LOG_ERROR("1. 头文件路径不正确");
        LOG_ERROR("2. 编译参数不兼容");
        LOG_ERROR("3. 源代码包含语法错误");
        LOG_ERROR("4. Clang版本不兼容");
        
        // 尝试简化的编译参数重新创建
        LOG_INFO("尝试使用简化的编译参数重新创建AST...");
        std::vector<std::string> simpleArgs = {
            "-std=c++17",
            "-xc++",
            "-Wno-everything",
            "-ferror-limit=0",
            "-fsyntax-only",
            "-I.",
            "-I./include",
            "-I./src"
        };
        
        astUnit = clang::tooling::buildASTFromCodeWithArgs(content, simpleArgs, filePath);
        if (astUnit) {
            LOG_INFO("使用简化参数成功创建AST单元");
        } else {
            LOG_ERROR("即使使用简化参数也无法创建AST单元");
        }
        
        return astUnit;
    }

    LOG_DEBUG_FMT("成功创建AST单元: %s", filePath.c_str());
    
    // 检查是否有诊断信息（编译错误/警告）
    auto& diagnostics = astUnit->getDiagnostics();
    if (diagnostics.hasErrorOccurred()) {
        LOG_WARNING_FMT("AST创建过程中有编译错误: %s", filePath.c_str());
        // 即使有错误，也可能能够进行部分分析
    }
    
    return astUnit;
}

Result<std::unique_ptr<ASTNodeInfo>> ASTAnalyzer::analyzeASTContext(clang::ASTContext& context,
                                                                    const std::string& filePath) {
    LOG_INFO_FMT("分析AST上下文: %s", filePath.c_str());
    
    // 特别处理ast_statement_analyzer.cpp文件
    bool isTargetFile = (filePath.find("ast_statement_analyzer.cpp") != std::string::npos);
    if (isTargetFile) {
        LOG_INFO_FMT("检测到目标文件: %s，启用详细调试", filePath.c_str());
        
        // 获取源代码管理器
        auto& sourceManager = context.getSourceManager();
        clang::FileID mainFileID = sourceManager.getMainFileID();
        if (const clang::FileEntry* fileEntry = sourceManager.getFileEntryForID(mainFileID)) {
            LOG_INFO_FMT("主文件ID对应的文件: %s", fileEntry->getName().str().c_str());
        }
        
        // 检查翻译单元的基本信息
        clang::TranslationUnitDecl* tu = context.getTranslationUnitDecl();
        LOG_INFO_FMT("翻译单元声明总数: %zu", std::distance(tu->decls_begin(), tu->decls_end()));
        
        // 预先检查第一批声明的类型
        int previewCount = 0;
        for (auto* decl : tu->decls()) {
            if (previewCount++ >= 10) break; // 只预览前10个
            
            auto loc = decl->getBeginLoc();
            bool isInMainFile = sourceManager.isInMainFile(loc);
            std::string declFilePath;
            if (loc.isValid()) {
                clang::FileID fileID = sourceManager.getFileID(loc);
                if (const clang::FileEntry* entry = sourceManager.getFileEntryForID(fileID)) {
                    declFilePath = entry->getName().str();
                }
            }
            
            if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
                LOG_INFO_FMT("预览函数声明 #%d: %s, 有函数体=%s, 主文件=%s, 位置文件=%s", 
                           previewCount, 
                           funcDecl->getNameAsString().c_str(),
                           funcDecl->hasBody() ? "是" : "否",
                           isInMainFile ? "是" : "否",
                           declFilePath.c_str());
            } else {
                LOG_INFO_FMT("预览声明 #%d: 类型=%s, 主文件=%s, 位置文件=%s", 
                           previewCount, 
                           decl->getDeclKindName(),
                           isInMainFile ? "是" : "否",
                           declFilePath.c_str());
            }
        }
    }

    // 创建函数分析器
    ASTFunctionAnalyzer functionAnalyzer(context, filePath, config_);
    // 创建根节点
    auto rootNode = std::make_unique<ASTNodeInfo>();
    rootNode->type = NodeType::UNKNOWN;  // 根节点类型设为FILE，表示文件级别的节点
    rootNode->name = std::filesystem::path(filePath).filename().string();
    rootNode->location.filePath = filePath;
    rootNode->location.line = 1;
    rootNode->location.column = 1;

    // 统计声明数量用于调试
    int totalDecls = 0;
    int funcDecls = 0;
    int classDecls = 0;
    int namespaceDecls = 0;

    // 遍历顶层声明
    LOG_INFO_FMT("开始遍历翻译单元声明，文件: %s", filePath.c_str());
    for (auto* decl : context.getTranslationUnitDecl()->decls()) {
        totalDecls++;
        
        // 获取声明的位置信息用于调试
        auto& sourceManager = context.getSourceManager();
        auto loc = decl->getBeginLoc();
        bool isInMainFile = sourceManager.isInMainFile(loc);
        
        // 获取文件路径进行更详细的检查
        std::string declFilePath;
        if (loc.isValid()) {
            clang::FileID fileID = sourceManager.getFileID(loc);
            if (const clang::FileEntry* fileEntry = sourceManager.getFileEntryForID(fileID)) {
                declFilePath = fileEntry->getName().str();
            }
        }
        
        LOG_INFO_FMT("声明 #%d: 类型=%s, 在主文件=%s, 文件路径=%s", 
                     totalDecls, decl->getDeclKindName(), isInMainFile ? "是" : "否", 
                     declFilePath.c_str());
        
        // 分析是否是函数声明
        if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            funcDecls++;
            bool hasBody = funcDecl->hasBody();
            
            // 放宽主文件检查条件 - 检查文件名是否匹配
            bool isRelevantFile = isInMainFile;
            if (!isRelevantFile && !declFilePath.empty()) {
                std::filesystem::path declPath(declFilePath);
                std::filesystem::path targetPath(filePath);
                // 检查文件名是否匹配（忽略路径差异）
                isRelevantFile = (declPath.filename() == targetPath.filename());
            }
            
            // 特别处理类方法（CXXMethodDecl）
            bool isMethodDecl = llvm::isa<clang::CXXMethodDecl>(funcDecl);
            std::string funcType = isMethodDecl ? "方法" : "函数";
            
            LOG_INFO_FMT("发现%s声明 #%d: %s, 有函数体=%s, 在主文件=%s, 文件匹配=%s", 
                         funcType.c_str(), funcDecls, funcDecl->getNameAsString().c_str(), 
                         hasBody ? "是" : "否", 
                         isInMainFile ? "是" : "否",
                         isRelevantFile ? "是" : "否");
            
            // 对于类方法，即使不在主文件中，如果文件名匹配也应该分析
            if (isMethodDecl && hasBody && !isRelevantFile && !declFilePath.empty()) {
                std::filesystem::path declPath(declFilePath);
                std::filesystem::path targetPath(filePath);
                if (declPath.filename() == targetPath.filename()) {
                    isRelevantFile = true;
                    LOG_INFO_FMT("类方法 %s 文件名匹配，强制设为相关文件", funcDecl->getNameAsString().c_str());
                }
            }
            
            // 检查函数是否在当前源文件中定义
            if (hasBody && isRelevantFile) {
                LOG_INFO_FMT("准备分析%s: %s", funcType.c_str(), funcDecl->getNameAsString().c_str());
                
                // 根据函数类型选择合适的分析方法
                if (isMethodDecl) {
                    // 对于类方法，使用analyzeMethodDecl
                    auto* methodDecl = llvm::cast<clang::CXXMethodDecl>(funcDecl);
                    auto result = functionAnalyzer.analyzeMethodDecl(methodDecl);
                    if (!result.hasError()) {
                        auto& funcNode = result.value();
                        if (funcNode) {
                            if (funcNode->hasLogging) {
                                rootNode->hasLogging = true;
                            }
                            rootNode->children.push_back(std::move(funcNode));
                            LOG_INFO_FMT("成功添加%s节点: %s", funcType.c_str(), funcDecl->getNameAsString().c_str());
                        }
                    } else {
                        LOG_WARNING_FMT("%s分析失败: %s, 错误: %s", 
                                       funcType.c_str(),
                                       funcDecl->getNameAsString().c_str(), 
                                       result.errorMessage().c_str());
                    }
                } else {
                    // 对于普通函数，使用analyzeFunctionDecl
                    auto result = functionAnalyzer.analyzeFunctionDecl(funcDecl);
                    if (!result.hasError()) {
                        auto& funcNode = result.value();
                        if (funcNode) {
                            if (funcNode->hasLogging) {
                                rootNode->hasLogging = true;
                            }
                            rootNode->children.push_back(std::move(funcNode));
                            LOG_INFO_FMT("成功添加%s节点: %s", funcType.c_str(), funcDecl->getNameAsString().c_str());
                        }
                    } else {
                        LOG_WARNING_FMT("%s分析失败: %s, 错误: %s", 
                                       funcType.c_str(),
                                       funcDecl->getNameAsString().c_str(), 
                                       result.errorMessage().c_str());
                    }
                }
            } else {
                LOG_INFO_FMT("跳过%s %s: 无函数体=%s, 文件不匹配=%s", 
                             funcType.c_str(),
                             funcDecl->getNameAsString().c_str(),
                             !hasBody ? "是" : "否",
                             !isRelevantFile ? "是" : "否");
            }
        }
        // 分析是否是类定义
        else if (auto* classDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
            classDecls++;
            bool hasDefinition = classDecl->hasDefinition();
            
            // 放宽主文件检查条件
            bool isRelevantFile = isInMainFile;
            if (!isRelevantFile && !declFilePath.empty()) {
                std::filesystem::path declPath(declFilePath);
                std::filesystem::path targetPath(filePath);
                isRelevantFile = (declPath.filename() == targetPath.filename());
            }
            
            LOG_INFO_FMT("发现类声明 #%d: %s, 有定义=%s, 在主文件=%s, 文件匹配=%s", 
                         classDecls, classDecl->getNameAsString().c_str(), 
                         hasDefinition ? "是" : "否", 
                         isInMainFile ? "是" : "否",
                         isRelevantFile ? "是" : "否");
            
            // 检查类是否在当前源文件中定义
            if (hasDefinition && isRelevantFile) {
                LOG_INFO_FMT("准备分析类: %s", classDecl->getNameAsString().c_str());
                // 遍历类中的方法
                int methodCount = 0;
                for (auto* method : classDecl->methods()) {
                    methodCount++;
                    if (method->hasBody()) {
                        LOG_INFO_FMT("分析类方法 #%d: %s::%s", 
                                     methodCount,
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
                                LOG_INFO_FMT("成功添加方法节点: %s::%s", 
                                           classDecl->getNameAsString().c_str(),
                                           method->getNameAsString().c_str());
                            }
                        } else {
                            LOG_WARNING_FMT("方法分析失败: %s::%s, 错误: %s", 
                                           classDecl->getNameAsString().c_str(),
                                           method->getNameAsString().c_str(),
                                           result.errorMessage().c_str());
                        }
                    } else {
                        LOG_INFO_FMT("跳过无函数体的方法: %s::%s", 
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                    }
                }
                LOG_INFO_FMT("类 %s 共有 %d 个方法", classDecl->getNameAsString().c_str(), methodCount);
            } else {
                LOG_INFO_FMT("跳过类 %s: 无定义=%s, 文件不匹配=%s", 
                             classDecl->getNameAsString().c_str(),
                             !hasDefinition ? "是" : "否",
                             !isRelevantFile ? "是" : "否");
            }
        }
        // 分析是否是命名空间
        else if (auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
            namespaceDecls++;
            
            // 放宽主文件检查条件
            bool isRelevantFile = isInMainFile;
            if (!isRelevantFile && !declFilePath.empty()) {
                std::filesystem::path declPath(declFilePath);
                std::filesystem::path targetPath(filePath);
                isRelevantFile = (declPath.filename() == targetPath.filename());
            }
            
            LOG_INFO_FMT("发现命名空间声明 #%d: %s, 在主文件=%s, 文件匹配=%s", 
                         namespaceDecls, namespaceDecl->getNameAsString().c_str(), 
                         isInMainFile ? "是" : "否",
                         isRelevantFile ? "是" : "否");
            
            // 检查命名空间是否在当前源文件中定义
            if (isRelevantFile) {
                LOG_INFO_FMT("准备分析命名空间: %s", namespaceDecl->getNameAsString().c_str());
                // 递归遍历命名空间中的声明
                analyzeNamespaceRecursively(namespaceDecl, namespaceDecl->getNameAsString(), filePath, 
                                          sourceManager, functionAnalyzer, rootNode);
            } else {
                LOG_INFO_FMT("跳过命名空间 %s: 文件不匹配", 
                             namespaceDecl->getNameAsString().c_str());
            }
        }
    }

    // 输出统计信息
    LOG_INFO_FMT("AST分析统计 - 文件: %s", filePath.c_str());
    LOG_INFO_FMT("  总声明数: %d", totalDecls);
    LOG_INFO_FMT("  函数声明数: %d", funcDecls);
    LOG_INFO_FMT("  类声明数: %d", classDecls);
    LOG_INFO_FMT("  命名空间声明数: %d", namespaceDecls);
    LOG_INFO_FMT("  识别的函数/方法节点数: %zu", rootNode->children.size());

    if (rootNode->children.empty()) {
        LOG_WARNING_FMT("警告: 文件 %s 中未识别到任何函数或方法", filePath.c_str());
        LOG_WARNING("可能的原因:");
        LOG_WARNING("1. 文件只包含声明，没有函数定义");
        LOG_WARNING("2. 编译参数不正确，导致AST解析失败");
        LOG_WARNING("3. 文件中的函数不在主文件范围内");
        LOG_WARNING("4. Clang解析时遇到了错误");
    }

    LOG_INFO_FMT("AST上下文分析完成，找到 %zu 个顶层函数/方法", rootNode->children.size());
    return makeSuccess(std::move(rootNode));
}

// 递归分析命名空间的辅助函数
void ASTAnalyzer::analyzeNamespaceRecursively(clang::NamespaceDecl* namespaceDecl, 
                                              const std::string& namespacePath,
                                              const std::string& filePath,
                                              clang::SourceManager& sourceManager,
                                              ASTFunctionAnalyzer& functionAnalyzer,
                                              std::unique_ptr<ASTNodeInfo>& rootNode) {
    LOG_INFO_FMT("递归分析命名空间: %s", namespacePath.c_str());
    
    int declCount = 0;
    for (auto* decl : namespaceDecl->decls()) {
        declCount++;
        
        // 检查声明的位置信息
        auto loc = decl->getBeginLoc();
        bool isInMainFile = sourceManager.isInMainFile(loc);
        std::string declFilePath;
        if (loc.isValid()) {
            clang::FileID fileID = sourceManager.getFileID(loc);
            if (const clang::FileEntry* entry = sourceManager.getFileEntryForID(fileID)) {
                declFilePath = entry->getName().str();
            }
        }
        
        // 放宽文件检查条件
        bool isRelevantFile = isInMainFile;
        if (!isRelevantFile && !declFilePath.empty()) {
            std::filesystem::path declPath(declFilePath);
            std::filesystem::path targetPath(filePath);
            isRelevantFile = (declPath.filename() == targetPath.filename());
        }
        
        LOG_INFO_FMT("命名空间%s中的声明 #%d: 类型=%s, 主文件=%s, 文件匹配=%s", 
                     namespacePath.c_str(), declCount, decl->getDeclKindName(),
                     isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
        
        if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            bool hasBody = funcDecl->hasBody();
            bool isMethodDecl = llvm::isa<clang::CXXMethodDecl>(funcDecl);
            std::string funcType = isMethodDecl ? "方法" : "函数";
            
            LOG_INFO_FMT("发现命名空间%s #%d: %s::%s, 有函数体=%s, 主文件=%s, 文件匹配=%s", 
                         funcType.c_str(), declCount, namespacePath.c_str(),
                         funcDecl->getNameAsString().c_str(),
                         hasBody ? "是" : "否", isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
            
            if (hasBody && isRelevantFile) {
                LOG_INFO_FMT("分析命名空间%s: %s::%s", 
                             funcType.c_str(), namespacePath.c_str(),
                             funcDecl->getNameAsString().c_str());
                
                // 根据函数类型选择合适的分析方法
                if (isMethodDecl) {
                    auto* methodDecl = llvm::cast<clang::CXXMethodDecl>(funcDecl);
                    auto result = functionAnalyzer.analyzeMethodDecl(methodDecl);
                    if (!result.hasError()) {
                        auto& funcNode = result.value();
                        if (funcNode) {
                            if (funcNode->hasLogging) {
                                rootNode->hasLogging = true;
                            }
                            rootNode->children.push_back(std::move(funcNode));
                            LOG_INFO_FMT("成功添加命名空间%s节点: %s::%s", 
                                       funcType.c_str(), namespacePath.c_str(),
                                       funcDecl->getNameAsString().c_str());
                        }
                    } else {
                        LOG_WARNING_FMT("命名空间%s分析失败: %s::%s, 错误: %s", 
                                       funcType.c_str(), namespacePath.c_str(),
                                       funcDecl->getNameAsString().c_str(),
                                       result.errorMessage().c_str());
                    }
                } else {
                    auto result = functionAnalyzer.analyzeFunctionDecl(funcDecl);
                    if (!result.hasError()) {
                        auto& funcNode = result.value();
                        if (funcNode) {
                            if (funcNode->hasLogging) {
                                rootNode->hasLogging = true;
                            }
                            rootNode->children.push_back(std::move(funcNode));
                            LOG_INFO_FMT("成功添加命名空间%s节点: %s::%s", 
                                       funcType.c_str(), namespacePath.c_str(),
                                       funcDecl->getNameAsString().c_str());
                        }
                    } else {
                        LOG_WARNING_FMT("命名空间%s分析失败: %s::%s, 错误: %s", 
                                       funcType.c_str(), namespacePath.c_str(),
                                       funcDecl->getNameAsString().c_str(),
                                       result.errorMessage().c_str());
                    }
                }
            } else {
                LOG_INFO_FMT("跳过命名空间%s %s::%s: 无函数体=%s, 文件不匹配=%s", 
                             funcType.c_str(), namespacePath.c_str(),
                             funcDecl->getNameAsString().c_str(),
                             !hasBody ? "是" : "否", !isRelevantFile ? "是" : "否");
            }
        }
        // 处理命名空间中的类
        else if (auto* classDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
            bool hasDefinition = classDecl->hasDefinition();
            
            LOG_INFO_FMT("发现命名空间中的类 #%d: %s::%s, 有定义=%s, 主文件=%s, 文件匹配=%s", 
                         declCount, namespacePath.c_str(),
                         classDecl->getNameAsString().c_str(),
                         hasDefinition ? "是" : "否", isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
            
            if (hasDefinition && isRelevantFile) {
                LOG_INFO_FMT("分析命名空间中的类: %s::%s", 
                             namespacePath.c_str(), classDecl->getNameAsString().c_str());
                for (auto* method : classDecl->methods()) {
                    if (method->hasBody()) {
                        auto result = functionAnalyzer.analyzeMethodDecl(method);
                        if (!result.hasError()) {
                            auto& methodNode = result.value();
                            if (methodNode) {
                                if (methodNode->hasLogging) {
                                    rootNode->hasLogging = true;
                                }
                                rootNode->children.push_back(std::move(methodNode));
                                LOG_INFO_FMT("成功添加命名空间类方法节点: %s::%s::%s", 
                                           namespacePath.c_str(),
                                           classDecl->getNameAsString().c_str(),
                                           method->getNameAsString().c_str());
                            }
                        } else {
                            LOG_WARNING_FMT("命名空间类方法分析失败: %s::%s::%s, 错误: %s", 
                                           namespacePath.c_str(),
                                           classDecl->getNameAsString().c_str(),
                                           method->getNameAsString().c_str(),
                                           result.errorMessage().c_str());
                        }
                    }
                }
            }
        }
        // 处理嵌套的命名空间（递归处理）
        else if (auto* nestedNamespace = llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
            LOG_INFO_FMT("发现嵌套命名空间 #%d: %s::%s, 主文件=%s, 文件匹配=%s", 
                         declCount, namespacePath.c_str(),
                         nestedNamespace->getNameAsString().c_str(),
                         isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
            
            if (isRelevantFile) {
                std::string nestedPath = namespacePath + "::" + nestedNamespace->getNameAsString();
                // 递归调用自己处理更深层的命名空间
                analyzeNamespaceRecursively(nestedNamespace, nestedPath, filePath, 
                                          sourceManager, functionAnalyzer, rootNode);
            }
        }
    }
    
    LOG_INFO_FMT("命名空间 %s 共有 %d 个声明", namespacePath.c_str(), declCount);
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
