/**
 * @file ast_analyzer.cpp
 * @brief AST分析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/ast_analyzer/file_ownership_validator.h>
#include <dlogcover/core/language_detector/language_detector.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/cmake_parser.h>
#include <dlogcover/config/compile_commands_manager.h>
#include <dlogcover/config/config_manager.h>

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
#include <iostream>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

ASTAnalyzer::ASTAnalyzer(const config::Config& config, const source_manager::SourceManager& sourceManager,
                         config::ConfigManager& configManager)
    : config_(config), sourceManager_(sourceManager), configManager_(configManager),
      compileManager_(&configManager.getCompileCommandsManager()),
      fileValidator_(std::make_unique<FileOwnershipValidator>()) {
    LOG_DEBUG("AST分析器初始化");
    
    // 配置文件归属验证器
    // 使用当前工作目录作为项目根目录
    try {
        std::string projectRoot = config_.project.directory;
        if (projectRoot.empty()) {
            projectRoot = std::filesystem::current_path().string();
        }
        fileValidator_->setProjectRoot(projectRoot);
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("无法获取当前工作目录: %s", e.what());
    }
    
    // 添加扫描目录作为包含目录
    for (const auto& dir : config_.scan.directories) {
        fileValidator_->addIncludeDirectory(dir);
    }
    
    // 添加排除模式
    fileValidator_->addExcludePattern(".*/tests/.*");
    fileValidator_->addExcludePattern(".*/test/.*");
    fileValidator_->addExcludePattern(".*/build/.*");
    fileValidator_->addExcludePattern(".*/cmake-build-.*/.*");
    
    // 添加配置中的排除模式
    for (const auto& exclude : config_.scan.exclude_patterns) {
        fileValidator_->addExcludePattern(exclude);
    }
    
    // 启用调试模式
    fileValidator_->setDebugMode(false);
}

ASTAnalyzer::~ASTAnalyzer() {
    LOG_DEBUG("AST分析器销毁");
    
    // 输出文件归属验证器的统计信息
    if (fileValidator_) {
        std::string stats = fileValidator_->getStatistics();
        LOG_INFO("文件归属验证器统计信息:");
        LOG_INFO(stats.c_str());
    }
}

Result<bool> ASTAnalyzer::analyze(const std::string& filePath) {
    auto detectedLang = language_detector::LanguageDetector::detectLanguage(filePath);
    if (detectedLang != language_detector::SourceLanguage::CPP) {
        return makeSuccess(true);
    }

    LOG_INFO_FMT("开始分析文件: %s", filePath.c_str());

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
    LOG_DEBUG_FMT("准备调用analyzeASTContext: %s", filePath.c_str());
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

    // 如果启用了并行模式，使用并行分析
    if (parallelEnabled_) {
        return analyzeAllParallel();
    }

    bool allSuccess = true;
    for (const auto& sourceFile : sourceManager_.getSourceFiles()) {
        auto detectedLang = language_detector::LanguageDetector::detectLanguage(sourceFile.path);
        if (detectedLang != language_detector::SourceLanguage::CPP) {
            continue;
        }

        auto result = analyze(sourceFile.path);
        if (result.hasError()) {
            LOG_ERROR_FMT("分析文件失败: %s, 错误: %s", sourceFile.path.c_str(), result.errorMessage().c_str());
            allSuccess = false;
        }
    }

    return makeSuccess(std::move(allSuccess));
}

Result<bool> ASTAnalyzer::analyzeAllParallel() {
    LOG_INFO("开始并行分析所有源文件");
    
    const auto& sourceFiles = sourceManager_.getSourceFiles();
    if (sourceFiles.empty()) {
        LOG_WARNING("没有找到需要分析的源文件");
        return makeSuccess(true);
    }
    
    // 确定线程数
    size_t numThreads = maxThreads_;
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4; // 默认4个线程
        }
    }
    
    // 限制线程数不超过文件数
    numThreads = std::min(numThreads, sourceFiles.size());
    
    LOG_INFO_FMT("使用 %zu 个线程并行分析 %zu 个文件", numThreads, sourceFiles.size());
    
    // 如果只有一个文件或一个线程，回退到串行处理
    if (numThreads <= 1 || sourceFiles.size() <= 1) {
        LOG_INFO("文件数量较少，使用串行处理");
        // 直接进行串行分析，避免递归调用analyzeAll()
        bool allSuccess = true;
        for (const auto& sourceFile : sourceFiles) {
            auto detectedLang = language_detector::LanguageDetector::detectLanguage(sourceFile.path);
            if (detectedLang != language_detector::SourceLanguage::CPP) {
                continue;
            }

            auto result = analyze(sourceFile.path);
            if (result.hasError()) {
                LOG_ERROR_FMT("串行分析文件失败: %s, 错误: %s", 
                             sourceFile.path.c_str(), result.errorMessage().c_str());
                allSuccess = false;
            }
        }
        return makeSuccess(std::move(allSuccess));
    }
    
    // 创建线程池
    threadPool_ = std::make_unique<utils::ThreadPool>(numThreads);
    
    // 使用RAII确保线程池正确关闭
    class ThreadPoolGuard {
        std::unique_ptr<utils::ThreadPool>& pool_;
    public:
        explicit ThreadPoolGuard(std::unique_ptr<utils::ThreadPool>& pool) : pool_(pool) {}
        ~ThreadPoolGuard() {
            if (pool_) {
                try {
                    pool_->shutdown();
                    pool_.reset();
                } catch (const std::exception& e) {
                    LOG_ERROR_FMT("线程池关闭异常: %s", e.what());
                }
            }
        }
    };
    
    ThreadPoolGuard poolGuard(threadPool_);
    
    // 提交分析任务
    std::vector<std::future<Result<bool>>> futures;
    std::atomic<size_t> processedCount{0};
    std::atomic<size_t> errorCount{0};
    
    futures.reserve(sourceFiles.size());
    
    try {
        for (const auto& sourceFile : sourceFiles) {
            auto detectedLang = language_detector::LanguageDetector::detectLanguage(sourceFile.path);
            if (detectedLang != language_detector::SourceLanguage::CPP) {
                continue;
            }

            futures.emplace_back(threadPool_->enqueue([this, sourceFile, &processedCount, &errorCount, &sourceFiles]() {
                auto result = this->analyzeSingleFile(sourceFile.path);
                
                size_t currentProcessed = processedCount.fetch_add(1) + 1;
                
                if (result.hasError()) {
                    errorCount.fetch_add(1);
                    LOG_ERROR_FMT("并行文件分析失败: %s, 错误: %s", 
                                 sourceFile.path.c_str(), result.errorMessage().c_str());
                } else {
                    LOG_DEBUG_FMT("并行文件分析成功: %s", sourceFile.path.c_str());
                }
                
                // 每处理10个文件报告一次进度
                if (currentProcessed % 10 == 0 || currentProcessed == sourceFiles.size()) {
                    LOG_INFO_FMT("分析进度: %zu/%zu (%.1f%%)", 
                               currentProcessed, sourceFiles.size(), 
                               (currentProcessed * 100.0) / sourceFiles.size());
                }
                
                return result;
            }));
        }
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("提交任务时发生异常: %s", e.what());
        return makeError<bool>(ASTAnalyzerError::COMPILATION_ERROR, "任务提交失败: " + std::string(e.what()));
    }
    
    // 等待所有任务完成，带超时检查
    bool allSuccess = true;
    const auto timeout = std::chrono::seconds(60);  // 60秒超时
    
    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            auto& future = futures[i];
            auto status = future.wait_for(timeout);
            
            if (status == std::future_status::timeout) {
                LOG_ERROR_FMT("任务 %zu 执行超时，可能存在死锁", i + 1);
                allSuccess = false;
                continue;
            }
            
            if (status == std::future_status::ready) {
                auto result = future.get();
                if (result.hasError()) {
                    allSuccess = false;
                }
            } else {
                LOG_ERROR_FMT("任务 %zu 状态异常", i + 1);
                allSuccess = false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR_FMT("获取分析结果时发生异常: %s", e.what());
            allSuccess = false;
        }
    }
    
    // ThreadPoolGuard 会自动关闭线程池
    
    size_t successCount = sourceFiles.size() - errorCount.load();
    LOG_INFO_FMT("并行分析完成，成功: %zu, 失败: %zu, 总计: %zu", 
                successCount, errorCount.load(), sourceFiles.size());
    
    return makeSuccess(std::move(allSuccess));
}

void ASTAnalyzer::setParallelMode(bool enabled, size_t maxThreads) {
    parallelEnabled_ = enabled;
    maxThreads_ = maxThreads;
    
    LOG_INFO_FMT("并行模式设置: %s, 最大线程数: %zu", 
                enabled ? "启用" : "禁用", maxThreads);
}

void ASTAnalyzer::enableCache(bool enabled, size_t maxCacheSize, size_t maxMemoryMB) {
    cacheEnabled_ = enabled;
    
    if (enabled) {
        astCache_ = std::make_unique<ASTCache>(maxCacheSize, maxMemoryMB);
        astCache_->setDebugMode(false); // 可以根据需要调整
        LOG_INFO_FMT("启用AST缓存，最大条目数: %zu, 最大内存: %zu MB", 
                    maxCacheSize, maxMemoryMB);
    } else {
        astCache_.reset();
        LOG_INFO("禁用AST缓存");
    }
}

std::string ASTAnalyzer::getCacheStatistics() const {
    if (astCache_) {
        return astCache_->getStatistics();
    }
    return "AST缓存未启用";
}

Result<bool> ASTAnalyzer::analyzeSingleFile(const std::string& filePath) {
    LOG_DEBUG_FMT("开始分析文件: %s", filePath.c_str());

    // 检查缓存
    if (cacheEnabled_ && astCache_ && astCache_->isCacheValid(filePath)) {
        auto cachedAST = astCache_->getCachedAST(filePath);
        if (cachedAST) {
            // 线程安全地保存缓存的AST节点信息
            {
                std::lock_guard<std::mutex> lock(astNodesMutex_);
                astNodes_[filePath] = std::move(cachedAST);
            }
            LOG_DEBUG_FMT("使用缓存的AST信息: %s", filePath.c_str());
            return makeSuccess(true);
        }
    }

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

    // 创建AST单元（每个线程使用独立的AST单元）
    auto astUnit = createASTUnit(filePath, content);
    if (!astUnit) {
        return makeError<bool>(ASTAnalyzerError::COMPILATION_ERROR, "创建AST单元失败: " + filePath);
    }

    // 分析AST上下文
    LOG_DEBUG_FMT("准备调用analyzeASTContext: %s", filePath.c_str());
    auto result = analyzeASTContext(astUnit->getASTContext(), filePath);
    if (result.hasError()) {
        return makeError<bool>(result.error(), result.errorMessage());
    }

    // 获取分析结果
    auto astInfo = std::move(result.value());
    
    // 缓存分析结果（创建拷贝用于缓存）
    if (cacheEnabled_ && astCache_ && astInfo) {
        auto astInfoCopy = std::make_unique<ASTNodeInfo>(*astInfo);
        astCache_->cacheAST(filePath, std::move(astInfoCopy));
    }

    // 线程安全地保存AST节点信息
    {
        std::lock_guard<std::mutex> lock(astNodesMutex_);
        astNodes_[filePath] = std::move(astInfo);
    }

    LOG_DEBUG_FMT("文件分析完成: %s", filePath.c_str());
    return makeSuccess(true);
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

void ASTAnalyzer::addExternalResult(const std::string& filePath, std::unique_ptr<ASTNodeInfo> result) {
    if (!result) {
        LOG_WARNING_FMT("尝试添加空的外部分析结果: %s", filePath.c_str());
        return;
    }
    
    LOG_DEBUG_FMT("添加外部分析结果: %s", filePath.c_str());
    
    // 线程安全地添加结果
    std::lock_guard<std::mutex> lock(astNodesMutex_);
    
    // 确保文件路径信息正确
    result->location.filePath = filePath;
    if (result->location.fileName.empty()) {
        result->location.fileName = filePath;
    }
    
    // 检查是否已有该文件的结果
    auto it = astNodes_.find(filePath);
    if (it != astNodes_.end()) {
        // 如果已有该文件的结果，将新结果作为子节点添加到现有结果中
        LOG_DEBUG_FMT("文件 %s 已有分析结果，将新结果添加为子节点", filePath.c_str());
        it->second->children.push_back(std::move(result));
    } else {
        // 如果没有该文件的结果，直接添加
        astNodes_[filePath] = std::move(result);
    }
    
    // 同时添加到results_向量中，保持接口一致性
    // 创建一个副本用于results_向量
    auto resultCopy = std::make_unique<ASTNodeInfo>(*astNodes_[filePath]);
    results_.push_back(std::move(resultCopy));
    
    LOG_DEBUG_FMT("成功添加外部分析结果: %s", filePath.c_str());
}

// 使用Clang/LLVM API创建AST单元
std::unique_ptr<clang::ASTUnit> ASTAnalyzer::createASTUnit(const std::string& filePath, const std::string& content) {
    LOG_DEBUG_FMT("创建文件的AST单元: %s", filePath.c_str());

    // 首先尝试使用CompileCommandsManager获取准确的编译参数
    std::vector<std::string> args;
    
    try {
        // 使用成员变量中的 compileManager_，避免重复创建和解析
        args = compileManager_->getCompilerArgs(filePath);
        
        if (!args.empty()) {
            LOG_DEBUG_FMT("成功获取文件的编译参数，数量: %zu", args.size());
            
            // 输出编译命令详情
            LOG_DEBUG("从CompileCommandsManager获取的编译参数:");
            for (size_t i = 0; i < args.size(); ++i) {
                LOG_DEBUG_FMT("  参数 #%zu: %s", i + 1, args[i].c_str());
            }
        } else {
            LOG_DEBUG_FMT("无法获取文件的编译参数: %s", filePath.c_str());
            LOG_DEBUG("将使用默认编译参数");
        }
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("CompileCommandsManager异常: %s", e.what());
        LOG_DEBUG("将使用默认编译参数");
    }
    
    // 编译参数现在通过 CompileCommandsManager 自动管理

    LOG_DEBUG_FMT("编译命令参数总数: %zu", args.size());
    LOG_DEBUG_FMT("完整编译命令: %s", llvm::join(args, " ").c_str());

    // 使用Clang的工具链创建AST
    LOG_DEBUG_FMT("开始为文件创建AST单元: %s", filePath.c_str());
    LOG_DEBUG_FMT("文件内容长度: %zu 字节", content.length());
    
    std::unique_ptr<clang::ASTUnit> astUnit = clang::tooling::buildASTFromCodeWithArgs(content, args, filePath);

    if (!astUnit) {
        LOG_ERROR_FMT("无法为文件创建AST单元: %s", filePath.c_str());
        LOG_ERROR("AST单元创建失败，这是导致函数无法识别的根本原因");
        
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
        };
        
        std::string projectDir = config_.project.directory;
        if (!projectDir.empty()) {
            simpleArgs.push_back("-I" + projectDir);
            simpleArgs.push_back("-I" + projectDir + "/include");
            simpleArgs.push_back("-I" + projectDir + "/src");
        }
        
        astUnit = clang::tooling::buildASTFromCodeWithArgs(content, simpleArgs, filePath);
        if (astUnit) {
            LOG_INFO("使用简化参数成功创建AST单元");
        } else {
            LOG_ERROR("即使使用简化参数也无法创建AST单元");
        }
        
        return astUnit;
    }

    LOG_DEBUG_FMT("成功创建AST单元: %s", filePath.c_str());
    
    // 检查AST单元的基本状态
    if (astUnit) {
        auto& astContext = astUnit->getASTContext();
        auto* translationUnit = astContext.getTranslationUnitDecl();
        
        if (translationUnit) {
            size_t declCount = std::distance(translationUnit->decls_begin(), translationUnit->decls_end());
            LOG_DEBUG_FMT("AST翻译单元包含 %zu 个顶层声明", declCount);
            
            if (declCount == 0) {
                LOG_WARNING_FMT("警告: AST翻译单元为空，这可能是函数无法识别的原因: %s", filePath.c_str());
            }
        } else {
            LOG_ERROR_FMT("错误: AST翻译单元为空指针: %s", filePath.c_str());
        }
    }
    
    // 检查是否有诊断信息（编译错误/警告）
    auto& diagnostics = astUnit->getDiagnostics();
    if (diagnostics.hasErrorOccurred()) {
        LOG_WARNING_FMT("AST创建过程中有编译错误: %s", filePath.c_str());
        
        // 输出详细的诊断信息
        LOG_DEBUG("检测到编译错误，这可能是函数无法识别的原因");
        LOG_DEBUG("建议检查:");
        LOG_DEBUG("1. 头文件路径是否正确");
        LOG_DEBUG("2. 编译宏定义是否完整");
        LOG_DEBUG("3. CMake参数是否正确解析");
        LOG_DEBUG("4. 项目依赖是否已安装");
        
        // 即使有错误，也可能能够进行部分分析
        LOG_INFO("尽管有编译错误，仍尝试进行AST分析");
    } else {
        LOG_DEBUG_FMT("AST创建无编译错误: %s", filePath.c_str());
    }
    
    return astUnit;
}

Result<std::unique_ptr<ASTNodeInfo>> ASTAnalyzer::analyzeASTContext(clang::ASTContext& context,
                                                                    const std::string& filePath) {
    LOG_DEBUG_FMT("开始分析AST上下文: %s", filePath.c_str());

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
    LOG_DEBUG("开始遍历顶层声明...");
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
        
        LOG_DEBUG_FMT("声明 #%d: 类型=%s, 在主文件=%s, 文件路径=%s", 
                     totalDecls, decl->getDeclKindName(), isInMainFile ? "是" : "否", 
                     declFilePath.c_str());
        
        // 分析是否是函数声明
        if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            funcDecls++;
            bool hasBody = funcDecl->hasBody();
            
            // 简化文件归属判断：只统计当前文件中真正定义的函数
            bool isRelevantFile = isInMainFile;
            
            // 过滤隐式函数（编译器生成的构造函数、析构函数等）
            if (funcDecl->isImplicit()) {
                LOG_DEBUG_FMT("跳过隐式函数: %s", funcDecl->getNameAsString().c_str());
                continue;
            }
            
            // 精确过滤宏展开函数：保留函数名来自宏但函数体不是宏展开的函数（如TEST_F）
            auto funcLoc = funcDecl->getNameInfo().getLoc();
            bool isNameMacroExpanded = funcLoc.isValid() && sourceManager.isMacroBodyExpansion(funcLoc);
            bool isBodyMacroExpanded = false;
            
            // 检查函数体是否来自宏展开
            if (funcDecl->hasBody()) {
                auto bodyLoc = funcDecl->getBody()->getBeginLoc();
                isBodyMacroExpanded = bodyLoc.isValid() && sourceManager.isMacroBodyExpansion(bodyLoc);
            }
            
            // 过滤完全由宏展开生成的函数，保留TEST_F等有实际函数体的函数
            if (isNameMacroExpanded && isBodyMacroExpanded) {
                LOG_DEBUG_FMT("跳过完全宏展开生成的函数: %s", funcDecl->getNameAsString().c_str());
                continue;
            }
            
            // 记录宏展开状态用于调试
            if (isNameMacroExpanded || isBodyMacroExpanded) {
                LOG_DEBUG_FMT("函数 %s: 名称宏展开=%s, 函数体宏展开=%s", 
                             funcDecl->getNameAsString().c_str(),
                             isNameMacroExpanded ? "是" : "否",
                             isBodyMacroExpanded ? "是" : "否");
            }
            
            // 特别处理类方法（CXXMethodDecl）
            bool isMethodDecl = llvm::isa<clang::CXXMethodDecl>(funcDecl);
            std::string funcType = isMethodDecl ? "方法" : "函数";
            
            LOG_DEBUG_FMT("发现%s声明 #%d: %s, 有函数体=%s, 在主文件=%s, 文件匹配=%s", 
                         funcType.c_str(), funcDecls, funcDecl->getNameAsString().c_str(), 
                         hasBody ? "是" : "否", 
                         isInMainFile ? "是" : "否",
                         isRelevantFile ? "是" : "否");
            
            // 类方法和普通函数使用相同的文件归属判断逻辑
            
            // 检查函数是否在当前源文件中定义
            if (hasBody && isRelevantFile) {
                LOG_DEBUG_FMT("准备分析%s: %s", funcType.c_str(), funcDecl->getNameAsString().c_str());
                
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
                            LOG_DEBUG_FMT("成功添加%s节点: %s", funcType.c_str(), funcDecl->getNameAsString().c_str());
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
                            LOG_DEBUG_FMT("成功添加%s节点: %s", funcType.c_str(), funcDecl->getNameAsString().c_str());
                        }
                    } else {
                        LOG_WARNING_FMT("%s分析失败: %s, 错误: %s", 
                                       funcType.c_str(),
                                       funcDecl->getNameAsString().c_str(), 
                                       result.errorMessage().c_str());
                    }
                }
            } else {
                LOG_DEBUG_FMT("跳过%s %s: 无函数体=%s, 文件不匹配=%s", 
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
            
            // 简化文件归属判断：只统计当前文件中真正定义的类
            bool isRelevantFile = isInMainFile;
            
            LOG_DEBUG_FMT("发现类声明 #%d: %s, 有定义=%s, 在主文件=%s, 文件匹配=%s", 
                         classDecls, classDecl->getNameAsString().c_str(), 
                         hasDefinition ? "是" : "否", 
                         isInMainFile ? "是" : "否",
                         isRelevantFile ? "是" : "否");
            
            // 检查类是否在当前源文件中定义
            if (hasDefinition && isRelevantFile) {
                LOG_DEBUG_FMT("准备分析类: %s", classDecl->getNameAsString().c_str());
                // 遍历类中的方法
                int methodCount = 0;
                for (auto* method : classDecl->methods()) {
                    methodCount++;
                    
                    // 过滤隐式方法（编译器生成的构造函数、析构函数等）
                    if (method->isImplicit()) {
                        LOG_DEBUG_FMT("跳过类中的隐式方法: %s::%s", 
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                        continue;
                    }
                    
                    // 精确过滤宏展开方法：保留方法名来自宏但方法体不是宏展开的方法（如TEST_F）
                    auto methodLoc = method->getNameInfo().getLoc();
                    bool isNameMacroExpanded = methodLoc.isValid() && sourceManager.isMacroBodyExpansion(methodLoc);
                    bool isBodyMacroExpanded = false;
                    
                    // 检查方法体是否来自宏展开
                    if (method->hasBody()) {
                        auto bodyLoc = method->getBody()->getBeginLoc();
                        isBodyMacroExpanded = bodyLoc.isValid() && sourceManager.isMacroBodyExpansion(bodyLoc);
                    }
                    
                    // 过滤完全由宏展开生成的方法，保留TEST_F等有实际方法体的方法
                    if (isNameMacroExpanded && isBodyMacroExpanded) {
                        LOG_DEBUG_FMT("跳过类中完全宏展开生成的方法: %s::%s", 
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                        continue;
                    }
                    
                    // 记录宏展开状态用于调试
                    if (isNameMacroExpanded || isBodyMacroExpanded) {
                        LOG_DEBUG_FMT("类方法 %s::%s: 名称宏展开=%s, 方法体宏展开=%s", 
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str(),
                                     isNameMacroExpanded ? "是" : "否",
                                     isBodyMacroExpanded ? "是" : "否");
                    }
                    
                    if (method->hasBody()) {
                        LOG_DEBUG_FMT("分析类方法 #%d: %s::%s", 
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
                                LOG_DEBUG_FMT("成功添加方法节点: %s::%s", 
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
                        LOG_DEBUG_FMT("跳过无函数体的方法: %s::%s", 
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                    }
                }
                LOG_DEBUG_FMT("类 %s 共有 %d 个方法", classDecl->getNameAsString().c_str(), methodCount);
            } else {
                LOG_DEBUG_FMT("跳过类 %s: 无定义=%s, 文件不匹配=%s", 
                             classDecl->getNameAsString().c_str(),
                             !hasDefinition ? "是" : "否",
                             !isRelevantFile ? "是" : "否");
            }
        }
        // 分析是否是命名空间
        else if (auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(decl)) {
            namespaceDecls++;
            
            // 简化文件归属判断：只统计当前文件中真正定义的命名空间
            bool isRelevantFile = isInMainFile;
            
            LOG_DEBUG_FMT("发现命名空间声明 #%d: %s, 在主文件=%s, 文件匹配=%s", 
                         namespaceDecls, namespaceDecl->getNameAsString().c_str(), 
                         isInMainFile ? "是" : "否",
                         isRelevantFile ? "是" : "否");
            
            // 检查命名空间是否在当前源文件中定义
            if (isRelevantFile) {
                LOG_DEBUG_FMT("准备分析命名空间: %s", namespaceDecl->getNameAsString().c_str());
                // 递归遍历命名空间中的声明
                analyzeNamespaceRecursively(namespaceDecl, namespaceDecl->getNameAsString(), filePath, 
                                          sourceManager, functionAnalyzer, rootNode);
            } else {
                LOG_DEBUG_FMT("跳过命名空间 %s: 文件不匹配", 
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
    LOG_DEBUG_FMT("递归分析命名空间: %s", namespacePath.c_str());
    
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
        
        // 简化文件归属判断：只统计当前文件中真正定义的声明
        bool isRelevantFile = isInMainFile;
        
        LOG_DEBUG_FMT("命名空间%s中的声明 #%d: 类型=%s, 主文件=%s, 文件匹配=%s", 
                     namespacePath.c_str(), declCount, decl->getDeclKindName(),
                     isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
        
        if (auto* funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
            bool hasBody = funcDecl->hasBody();
            
            // 过滤隐式函数（编译器生成的构造函数、析构函数等）
            if (funcDecl->isImplicit()) {
                LOG_DEBUG_FMT("跳过命名空间中的隐式函数: %s::%s", 
                             namespacePath.c_str(), funcDecl->getNameAsString().c_str());
                continue;
            }
            
            // 精确过滤宏展开函数：保留函数名来自宏但函数体不是宏展开的函数（如TEST_F）
            auto funcLoc = funcDecl->getNameInfo().getLoc();
            bool isNameMacroExpanded = funcLoc.isValid() && sourceManager.isMacroBodyExpansion(funcLoc);
            bool isBodyMacroExpanded = false;
            
            // 检查函数体是否来自宏展开
            if (funcDecl->hasBody()) {
                auto bodyLoc = funcDecl->getBody()->getBeginLoc();
                isBodyMacroExpanded = bodyLoc.isValid() && sourceManager.isMacroBodyExpansion(bodyLoc);
            }
            
            // 过滤完全由宏展开生成的函数，保留TEST_F等有实际函数体的函数
            if (isNameMacroExpanded && isBodyMacroExpanded) {
                LOG_DEBUG_FMT("跳过命名空间中完全宏展开生成的函数: %s::%s", 
                             namespacePath.c_str(), funcDecl->getNameAsString().c_str());
                continue;
            }
            
            // 记录宏展开状态用于调试
            if (isNameMacroExpanded || isBodyMacroExpanded) {
                LOG_DEBUG_FMT("命名空间函数 %s::%s: 名称宏展开=%s, 函数体宏展开=%s", 
                             namespacePath.c_str(), funcDecl->getNameAsString().c_str(),
                             isNameMacroExpanded ? "是" : "否",
                             isBodyMacroExpanded ? "是" : "否");
            }
            
            bool isMethodDecl = llvm::isa<clang::CXXMethodDecl>(funcDecl);
            std::string funcType = isMethodDecl ? "方法" : "函数";
            
            LOG_DEBUG_FMT("发现命名空间%s #%d: %s::%s, 有函数体=%s, 主文件=%s, 文件匹配=%s", 
                         funcType.c_str(), declCount, namespacePath.c_str(),
                         funcDecl->getNameAsString().c_str(),
                         hasBody ? "是" : "否", isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
            
            if (hasBody && isRelevantFile) {
                LOG_DEBUG_FMT("分析命名空间%s: %s::%s", 
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
                            LOG_DEBUG_FMT("成功添加命名空间%s节点: %s::%s", 
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
                            LOG_DEBUG_FMT("成功添加命名空间%s节点: %s::%s", 
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
                LOG_DEBUG_FMT("跳过命名空间%s %s::%s: 无函数体=%s, 文件不匹配=%s", 
                             funcType.c_str(), namespacePath.c_str(),
                             funcDecl->getNameAsString().c_str(),
                             !hasBody ? "是" : "否", !isRelevantFile ? "是" : "否");
            }
        }
        // 处理命名空间中的类
        else if (auto* classDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
            bool hasDefinition = classDecl->hasDefinition();
            
            LOG_DEBUG_FMT("发现命名空间中的类 #%d: %s::%s, 有定义=%s, 主文件=%s, 文件匹配=%s", 
                         declCount, namespacePath.c_str(),
                         classDecl->getNameAsString().c_str(),
                         hasDefinition ? "是" : "否", isInMainFile ? "是" : "否", isRelevantFile ? "是" : "否");
            
            if (hasDefinition && isRelevantFile) {
                LOG_DEBUG_FMT("分析命名空间中的类: %s::%s", 
                             namespacePath.c_str(), classDecl->getNameAsString().c_str());
                for (auto* method : classDecl->methods()) {
                    // 过滤隐式方法（编译器生成的构造函数、析构函数等）
                    if (method->isImplicit()) {
                        LOG_DEBUG_FMT("跳过命名空间类中的隐式方法: %s::%s::%s", 
                                     namespacePath.c_str(),
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                        continue;
                    }
                    
                    // 精确过滤宏展开方法：保留方法名来自宏但方法体不是宏展开的方法（如TEST_F）
                    auto methodLoc = method->getNameInfo().getLoc();
                    bool isNameMacroExpanded = methodLoc.isValid() && sourceManager.isMacroBodyExpansion(methodLoc);
                    bool isBodyMacroExpanded = false;
                    
                    // 检查方法体是否来自宏展开
                    if (method->hasBody()) {
                        auto bodyLoc = method->getBody()->getBeginLoc();
                        isBodyMacroExpanded = bodyLoc.isValid() && sourceManager.isMacroBodyExpansion(bodyLoc);
                    }
                    
                    // 过滤完全由宏展开生成的方法，保留TEST_F等有实际方法体的方法
                    if (isNameMacroExpanded && isBodyMacroExpanded) {
                        LOG_DEBUG_FMT("跳过命名空间类中完全宏展开生成的方法: %s::%s::%s", 
                                     namespacePath.c_str(),
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str());
                        continue;
                    }
                    
                    // 记录宏展开状态用于调试
                    if (isNameMacroExpanded || isBodyMacroExpanded) {
                        LOG_DEBUG_FMT("命名空间类方法 %s::%s::%s: 名称宏展开=%s, 方法体宏展开=%s", 
                                     namespacePath.c_str(),
                                     classDecl->getNameAsString().c_str(),
                                     method->getNameAsString().c_str(),
                                     isNameMacroExpanded ? "是" : "否",
                                     isBodyMacroExpanded ? "是" : "否");
                    }
                    
                    if (method->hasBody()) {
                        auto result = functionAnalyzer.analyzeMethodDecl(method);
                        if (!result.hasError()) {
                            auto& methodNode = result.value();
                            if (methodNode) {
                                if (methodNode->hasLogging) {
                                    rootNode->hasLogging = true;
                                }
                                rootNode->children.push_back(std::move(methodNode));
                                LOG_DEBUG_FMT("成功添加命名空间类方法节点: %s::%s::%s", 
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
            LOG_DEBUG_FMT("发现嵌套命名空间 #%d: %s::%s, 主文件=%s, 文件匹配=%s", 
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
    
    LOG_DEBUG_FMT("命名空间 %s 共有 %d 个声明", namespacePath.c_str(), declCount);
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

const std::vector<std::unique_ptr<ASTNodeInfo>>& ASTAnalyzer::getResults() const {
    return results_;
}

void ASTAnalyzer::clear() {
    LOG_DEBUG("清空AST分析器结果");
    results_.clear();
    astNodes_.clear();
    currentASTUnit_.reset();
    if (astCache_) {
        astCache_->clearCache();
    }
}

std::vector<std::string> ASTAnalyzer::getCompilerArgs() const {
    LOG_DEBUG("获取编译参数");
    
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
    
    // 基本系统定义
    args.push_back("-D__GNUG__");
    args.push_back("-D__linux__");
    args.push_back("-D__x86_64__");
    args.push_back("-D_GNU_SOURCE");
    args.push_back("-D__STDC_CONSTANT_MACROS");
    args.push_back("-D__STDC_FORMAT_MACROS");
    args.push_back("-D__STDC_LIMIT_MACROS");
    
    // Qt项目定义和自定义参数现在通过 CompileCommandsManager 自动管理
    
    LOG_DEBUG_FMT("回退编译参数总数: %zu", args.size());
    return args;
}

}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
