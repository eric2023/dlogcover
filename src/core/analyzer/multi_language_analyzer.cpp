/**
 * @file multi_language_analyzer.cpp
 * @brief 多语言分析器框架实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/analyzer/multi_language_analyzer.h>
#include <dlogcover/core/analyzer/cpp_analyzer_adapter.h>
#include <dlogcover/core/analyzer/go_analyzer.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/log_utils.h>

#include <thread>
#include <algorithm>

namespace dlogcover {
namespace core {
namespace analyzer {

MultiLanguageAnalyzer::MultiLanguageAnalyzer(const config::Config& config, 
                                           const source_manager::SourceManager& sourceManager,
                                           config::ConfigManager& configManager)
    : config_(config), sourceManager_(sourceManager), configManager_(configManager) {
    LOG_INFO("初始化多语言分析器框架");
    
    // 根据配置确定分析模式
    determineAnalysisMode();
    
    // 初始化所需的分析器
    initializeRequiredAnalyzers();
    
    LOG_INFO_FMT("多语言分析器框架初始化完成，模式: %s，支持 %zu 种语言", 
                 getAnalysisModeString(analysisMode_).c_str(), analyzers_.size());
}

MultiLanguageAnalyzer::~MultiLanguageAnalyzer() {
    LOG_DEBUG("销毁多语言分析器框架");
}

ast_analyzer::Result<bool> MultiLanguageAnalyzer::analyzeFile(const std::string& filePath) {
    LOG_DEBUG_FMT("多语言分析器分析文件: %s", filePath.c_str());
    
    // 检测文件语言类型
    auto language = language_detector::LanguageDetector::detectLanguage(filePath);
    
    // 更新统计信息
    statistics_.totalFiles++;
    
    // 根据语言类型路由到相应的分析器
    switch (language) {
        case language_detector::SourceLanguage::CPP: {
            LOG_DEBUG_FMT("路由到C++分析器: %s", filePath.c_str());
            statistics_.cppFiles++;
            
            auto it = analyzers_.find(language);
            if (it != analyzers_.end() && it->second) {
                auto result = it->second->analyze(filePath);
                updateStatistics(language, !result.hasError());
                return result;
            } else {
                LOG_ERROR("C++分析器未初始化");
                updateStatistics(language, false);
                return ast_analyzer::makeError<bool>(
                    ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
                    "C++分析器未初始化"
                );
            }
        }
        
        case language_detector::SourceLanguage::GO: {
            LOG_DEBUG_FMT("路由到Go分析器: %s", filePath.c_str());
            statistics_.goFiles++;
            
            auto it = analyzers_.find(language);
            if (it != analyzers_.end() && it->second) {
                if (it->second->isEnabled()) {
                    auto result = it->second->analyze(filePath);
                    updateStatistics(language, !result.hasError());
                    return result;
                } else {
                    LOG_DEBUG_FMT("Go语言支持未启用，跳过文件: %s", filePath.c_str());
                    return ast_analyzer::makeSuccess(true);
                }
            } else {
                LOG_WARNING_FMT("Go分析器未初始化，跳过文件: %s", filePath.c_str());
                return ast_analyzer::makeSuccess(true);
            }
        }
        
        case language_detector::SourceLanguage::UNKNOWN:
        default: {
            LOG_DEBUG_FMT("未知文件类型，跳过分析: %s", filePath.c_str());
            statistics_.unknownFiles++;
            return ast_analyzer::makeSuccess(true);
        }
    }
}

ast_analyzer::Result<bool> MultiLanguageAnalyzer::analyzeAll() {
    LOG_INFO_FMT("多语言分析器开始分析所有文件，模式: %s", getAnalysisModeString(analysisMode_).c_str());
    
    // 根据分析模式选择最优策略
    switch (analysisMode_) {
        case AnalysisMode::CPP_ONLY:
            return analyzeCppOnlyMode();
            
        case AnalysisMode::GO_ONLY:
            return analyzeGoOnlyMode();
            
        case AnalysisMode::AUTO_DETECT:
            return analyzeAutoDetectMode();
            
        default:
            return ast_analyzer::makeError<bool>(
                ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
                "未知的分析模式"
            );
    }
}

ast_analyzer::Result<bool> MultiLanguageAnalyzer::analyzeAllParallel() {
    LOG_INFO("多语言分析器开始并行分析所有文件");
    
    // 根据分析模式选择并行策略
    switch (analysisMode_) {
        case AnalysisMode::CPP_ONLY: {
            // C++模式使用原有的并行分析
            auto cppAnalyzer = dynamic_cast<CppAnalyzerAdapter*>(getCppAnalyzer());
            if (cppAnalyzer) {
                return cppAnalyzer->getUnderlyingAnalyzer()->analyzeAllParallel();
            }
            return analyzeCppOnlyMode();
        }
        
        case AnalysisMode::GO_ONLY: {
            // Go模式使用Go分析器的并行功能
            auto goAnalyzer = getGoAnalyzer();
            if (goAnalyzer) {
                goAnalyzer->setParallelMode(true, getOptimalThreadCount());
                return analyzeGoOnlyMode();
            }
            return analyzeGoOnlyMode();
        }
        
        case AnalysisMode::AUTO_DETECT:
        default:
            // 自动检测模式暂时使用串行分析
            LOG_DEBUG("自动检测模式使用串行分析");
            return analyzeAll();
    }
}

std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> MultiLanguageAnalyzer::getAllResults() const {
    LOG_DEBUG("合并所有语言的分析结果");
    
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> allResults;
    
    // 合并所有分析器的结果
    for (const auto& [language, analyzer] : analyzers_) {
        if (analyzer) {
            const auto& results = analyzer->getResults();
            for (const auto& result : results) {
                if (result) {
                    // 创建结果的副本
                    auto resultCopy = std::make_unique<ast_analyzer::ASTNodeInfo>(*result);
                    allResults.push_back(std::move(resultCopy));
                }
            }
        }
    }
    
    LOG_DEBUG_FMT("合并完成，共 %zu 个分析结果", allResults.size());
    return allResults;
}

void MultiLanguageAnalyzer::clearAll() {
    LOG_DEBUG("清空所有分析器结果");
    
    for (const auto& [language, analyzer] : analyzers_) {
        if (analyzer) {
            analyzer->clear();
        }
    }
    
    // 重置统计信息
    statistics_ = AnalysisStatistics{};
    
    LOG_DEBUG("所有分析器结果已清空");
}

ILanguageAnalyzer* MultiLanguageAnalyzer::getAnalyzer(language_detector::SourceLanguage language) const {
    auto it = analyzers_.find(language);
    return (it != analyzers_.end()) ? it->second.get() : nullptr;
}

ILanguageAnalyzer* MultiLanguageAnalyzer::getCppAnalyzer() const {
    return getAnalyzer(language_detector::SourceLanguage::CPP);
}

ILanguageAnalyzer* MultiLanguageAnalyzer::getGoAnalyzer() const {
    return getAnalyzer(language_detector::SourceLanguage::GO);
}

std::vector<language_detector::SourceLanguage> MultiLanguageAnalyzer::getSupportedLanguages() const {
    std::vector<language_detector::SourceLanguage> languages;
    
    for (const auto& [language, analyzer] : analyzers_) {
        if (analyzer) {
            languages.push_back(language);
        }
    }
    
    return languages;
}

std::string MultiLanguageAnalyzer::getStatistics() const {
    std::string stats = "多语言分析统计:\n";
    stats += "  分析模式: " + getAnalysisModeString(analysisMode_) + "\n";
    stats += "  总文件数: " + std::to_string(statistics_.totalFiles) + "\n";
    stats += "  C++文件: " + std::to_string(statistics_.cppFiles) + "\n";
    stats += "  Go文件: " + std::to_string(statistics_.goFiles) + "\n";
    stats += "  未知文件: " + std::to_string(statistics_.unknownFiles) + "\n";
    stats += "  成功分析: " + std::to_string(statistics_.successfulAnalyses) + "\n";
    stats += "  失败分析: " + std::to_string(statistics_.failedAnalyses) + "\n";
    
    // 添加各语言分析器的统计信息
    for (const auto& [language, analyzer] : analyzers_) {
        if (analyzer) {
            stats += "  " + analyzer->getLanguageName() + ": " + analyzer->getStatistics() + "\n";
        }
    }
    
    return stats;
}

void MultiLanguageAnalyzer::setParallelMode(bool enabled, size_t maxThreads) {
    LOG_DEBUG_FMT("设置多语言分析器并行模式: enabled=%s, maxThreads=%zu", 
                  enabled ? "true" : "false", maxThreads);
    
    // 将并行设置传递给支持的分析器
    auto cppAnalyzer = dynamic_cast<CppAnalyzerAdapter*>(getCppAnalyzer());
    if (cppAnalyzer) {
        cppAnalyzer->setParallelMode(enabled, maxThreads);
    }
    
    auto goAnalyzer = getGoAnalyzer();
    if (goAnalyzer) {
        goAnalyzer->setParallelMode(enabled, maxThreads);
    }
}

void MultiLanguageAnalyzer::enableCache(bool enabled, size_t maxCacheSize, size_t maxMemoryMB) {
    LOG_DEBUG_FMT("设置多语言分析器缓存: enabled=%s, maxCacheSize=%zu, maxMemoryMB=%zu", 
                  enabled ? "true" : "false", maxCacheSize, maxMemoryMB);
    
    // 将缓存设置传递给支持的分析器
    auto cppAnalyzer = dynamic_cast<CppAnalyzerAdapter*>(getCppAnalyzer());
    if (cppAnalyzer) {
        cppAnalyzer->enableCache(enabled, maxCacheSize, maxMemoryMB);
    }
    
    // Go分析器的缓存将在后续实现
}

bool MultiLanguageAnalyzer::isLanguageEnabled(language_detector::SourceLanguage language) const {
    auto analyzer = getAnalyzer(language);
    return analyzer ? analyzer->isEnabled() : false;
}

std::string MultiLanguageAnalyzer::getAnalysisModeString(AnalysisMode mode) {
    switch (mode) {
        case AnalysisMode::CPP_ONLY:
            return "CPP_ONLY";
        case AnalysisMode::GO_ONLY:
            return "GO_ONLY";
        case AnalysisMode::AUTO_DETECT:
            return "AUTO_DETECT";
        default:
            return "UNKNOWN";
    }
}

void MultiLanguageAnalyzer::determineAnalysisMode() {
    std::string modeStr = config_.analysis.mode;
    
    if (modeStr == "cpp_only") {
        analysisMode_ = AnalysisMode::CPP_ONLY;
        LOG_INFO("使用纯C++分析模式 - 保持原有性能优势");
    } else if (modeStr == "go_only") {
        analysisMode_ = AnalysisMode::GO_ONLY;
        LOG_INFO("使用纯Go分析模式 - 启用多线程优化");
    } else if (modeStr == "auto_detect") {
        analysisMode_ = AnalysisMode::AUTO_DETECT;
        LOG_INFO("使用自动检测分析模式 - 支持混合项目");
    } else {
        // 未知配置，默认C++模式
        analysisMode_ = AnalysisMode::CPP_ONLY;
        LOG_WARNING_FMT("未知的分析模式配置: %s，使用默认C++模式", modeStr.c_str());
    }
}

void MultiLanguageAnalyzer::initializeRequiredAnalyzers() {
    switch (analysisMode_) {
        case AnalysisMode::CPP_ONLY:
            // 只初始化C++分析器
            if (auto cppAnalyzer = createCppAnalyzer()) {
                analyzers_[language_detector::SourceLanguage::CPP] = std::move(cppAnalyzer);
                LOG_INFO("C++分析器初始化成功");
            } else {
                LOG_ERROR("C++分析器初始化失败");
            }
            break;
            
        case AnalysisMode::GO_ONLY:
            // 只初始化Go分析器
            if (auto goAnalyzer = createGoAnalyzer()) {
                analyzers_[language_detector::SourceLanguage::GO] = std::move(goAnalyzer);
                LOG_INFO("Go分析器初始化成功");
            } else {
                LOG_ERROR("Go分析器初始化失败");
            }
            break;
            
        case AnalysisMode::AUTO_DETECT:
            // 先进行语言检测，再初始化对应分析器
            detectProjectLanguage();
            if (detectedLanguage_ == language_detector::SourceLanguage::CPP) {
                if (auto cppAnalyzer = createCppAnalyzer()) {
                    analyzers_[language_detector::SourceLanguage::CPP] = std::move(cppAnalyzer);
                    LOG_INFO("检测到C++项目，初始化C++分析器");
                }
            } else if (detectedLanguage_ == language_detector::SourceLanguage::GO) {
                if (auto goAnalyzer = createGoAnalyzer()) {
                    analyzers_[language_detector::SourceLanguage::GO] = std::move(goAnalyzer);
                    LOG_INFO("检测到Go项目，初始化Go分析器");
                }
            } else {
                // 混合项目或未知，初始化所有分析器
                LOG_INFO("检测到混合项目，初始化所有分析器");
                initializeAnalyzers();
            }
            break;
    }
}

ast_analyzer::Result<bool> MultiLanguageAnalyzer::analyzeCppOnlyMode() {
    LOG_INFO("开始纯C++项目分析 - 保持原有性能优势");
    
    auto cppAnalyzer = dynamic_cast<CppAnalyzerAdapter*>(getCppAnalyzer());
    if (!cppAnalyzer) {
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
            "C++分析器未初始化"
        );
    }
    
    // 直接调用原有的C++批量分析逻辑，绕过多语言框架开销
    return cppAnalyzer->getUnderlyingAnalyzer()->analyzeAll();
}

ast_analyzer::Result<bool> MultiLanguageAnalyzer::analyzeGoOnlyMode() {
    LOG_INFO("开始纯Go项目分析 - 启用多线程优化");
    
    auto goAnalyzer = getGoAnalyzer();
    if (!goAnalyzer) {
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
            "Go分析器未初始化"
        );
    }
    
    // 收集所有Go文件
    std::vector<std::string> goFiles = collectFilesByLanguage(language_detector::SourceLanguage::GO);
    if (goFiles.empty()) {
        LOG_WARNING("未找到Go源文件");
        return ast_analyzer::makeSuccess(true);
    }
    
    LOG_INFO_FMT("找到 %zu 个Go文件，开始批量分析", goFiles.size());
    
    // 启用Go分析器的多线程模式
    goAnalyzer->setParallelMode(true, getOptimalThreadCount());
    
    // 批量分析Go文件
    bool hasErrors = false;
    for (const auto& filePath : goFiles) {
        auto result = goAnalyzer->analyze(filePath);
        if (result.hasError()) {
            LOG_ERROR_FMT("Go文件分析失败: %s, 错误: %s", 
                         filePath.c_str(), result.errorMessage().c_str());
            hasErrors = true;
        }
    }
    
    if (hasErrors) {
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::ANALYSIS_ERROR,
            "部分Go文件分析失败"
        );
    }
    
    return ast_analyzer::makeSuccess(true);
}

ast_analyzer::Result<bool> MultiLanguageAnalyzer::analyzeAutoDetectMode() {
    LOG_INFO("开始自动检测分析模式 - 支持混合项目");
    
    // 按语言分组文件
    std::vector<std::string> cppFiles = collectFilesByLanguage(language_detector::SourceLanguage::CPP);
    std::vector<std::string> goFiles = collectFilesByLanguage(language_detector::SourceLanguage::GO);
    
    LOG_INFO_FMT("检测到 %zu 个C++文件，%zu 个Go文件", cppFiles.size(), goFiles.size());
    
    bool hasErrors = false;
    
    // 分析C++文件
    if (!cppFiles.empty()) {
        auto cppAnalyzer = getCppAnalyzer();
        if (cppAnalyzer) {
            for (const auto& filePath : cppFiles) {
                auto result = cppAnalyzer->analyze(filePath);
                if (result.hasError()) {
                    LOG_ERROR_FMT("C++文件分析失败: %s, 错误: %s", 
                                 filePath.c_str(), result.errorMessage().c_str());
                    hasErrors = true;
                }
            }
        }
    }
    
    // 分析Go文件
    if (!goFiles.empty()) {
        auto goAnalyzer = getGoAnalyzer();
        if (goAnalyzer) {
            for (const auto& filePath : goFiles) {
                auto result = goAnalyzer->analyze(filePath);
                if (result.hasError()) {
                    LOG_ERROR_FMT("Go文件分析失败: %s, 错误: %s", 
                                 filePath.c_str(), result.errorMessage().c_str());
                    hasErrors = true;
                }
            }
        }
    }
    
    if (hasErrors) {
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::ANALYSIS_ERROR,
            "部分文件分析失败"
        );
    }
    
    return ast_analyzer::makeSuccess(true);
}

void MultiLanguageAnalyzer::detectProjectLanguage() {
    LOG_DEBUG("开始项目语言抽样检测");
    
    const auto& sourceFiles = sourceManager_.getSourceFiles();
    if (sourceFiles.empty()) {
        detectedLanguage_ = language_detector::SourceLanguage::CPP; // 默认C++
        return;
    }
    
    size_t sampleSize = std::min(
        config_.analysis.auto_detection.sample_size,
        sourceFiles.size()
    );
    
    std::map<language_detector::SourceLanguage, size_t> languageCounts;
    
    // 抽样检测
    for (size_t i = 0; i < sampleSize; ++i) {
        const auto& fileInfo = sourceFiles[i];
        auto language = language_detector::LanguageDetector::detectLanguage(fileInfo.path);
        languageCounts[language]++;
    }
    
    // 找出主要语言
    auto maxIt = std::max_element(languageCounts.begin(), languageCounts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    if (maxIt != languageCounts.end()) {
        double confidence = static_cast<double>(maxIt->second) / sampleSize;
        if (confidence >= config_.analysis.auto_detection.confidence_threshold) {
            detectedLanguage_ = maxIt->first;
            LOG_INFO_FMT("检测到项目主要语言: %s (置信度: %.2f)", 
                       language_detector::LanguageDetector::getLanguageName(detectedLanguage_).c_str(),
                       confidence);
        } else {
            detectedLanguage_ = language_detector::SourceLanguage::UNKNOWN; // 混合项目
            LOG_INFO_FMT("语言检测置信度不足 (%.2f < %.2f)，判定为混合项目", 
                        confidence, config_.analysis.auto_detection.confidence_threshold);
        }
    } else {
        detectedLanguage_ = language_detector::SourceLanguage::CPP; // 默认C++
    }
}

std::vector<std::string> MultiLanguageAnalyzer::collectFilesByLanguage(language_detector::SourceLanguage language) {
    std::vector<std::string> files;
    const auto& sourceFiles = sourceManager_.getSourceFiles();
    
    for (const auto& fileInfo : sourceFiles) {
        auto detectedLang = language_detector::LanguageDetector::detectLanguage(fileInfo.path);
        if (detectedLang == language) {
            files.push_back(fileInfo.path);
        }
    }
    
    return files;
}

size_t MultiLanguageAnalyzer::getOptimalThreadCount() {
    // 根据文件数量和系统资源确定最优线程数
    size_t hwThreads = std::thread::hardware_concurrency();
    size_t fileCount = sourceManager_.getSourceFiles().size();
    
    // 避免线程过多导致的上下文切换开销
    size_t optimalThreads = std::min({hwThreads, fileCount / 10 + 1, static_cast<size_t>(8)});
    
    // 至少使用1个线程
    return std::max(optimalThreads, static_cast<size_t>(1));
}

void MultiLanguageAnalyzer::initializeAnalyzers() {
    LOG_DEBUG("初始化所有语言分析器");
    
    // 创建C++分析器（始终创建，保持现有功能）
    auto cppAnalyzer = createCppAnalyzer();
    if (cppAnalyzer) {
        analyzers_[language_detector::SourceLanguage::CPP] = std::move(cppAnalyzer);
        LOG_INFO("C++分析器初始化成功");
    } else {
        LOG_ERROR("C++分析器初始化失败");
    }
    
    // 创建Go分析器（仅在启用时创建）
    if (config_.go.enabled) {
        auto goAnalyzer = createGoAnalyzer();
        if (goAnalyzer) {
            analyzers_[language_detector::SourceLanguage::GO] = std::move(goAnalyzer);
            LOG_INFO("Go分析器初始化成功");
        } else {
            LOG_WARNING("Go分析器初始化失败");
        }
    } else {
        LOG_DEBUG("Go语言支持未启用，跳过Go分析器初始化");
    }
}

std::unique_ptr<ILanguageAnalyzer> MultiLanguageAnalyzer::createCppAnalyzer() {
    try {
        return std::make_unique<CppAnalyzerAdapter>(config_, sourceManager_, configManager_);
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("创建C++分析器失败: %s", e.what());
        return nullptr;
    }
}

std::unique_ptr<ILanguageAnalyzer> MultiLanguageAnalyzer::createGoAnalyzer() {
    try {
        return std::make_unique<GoAnalyzer>(config_);
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("创建Go分析器失败: %s", e.what());
        return nullptr;
    }
}

void MultiLanguageAnalyzer::updateStatistics(language_detector::SourceLanguage language, bool success) {
    if (success) {
        statistics_.successfulAnalyses++;
    } else {
        statistics_.failedAnalyses++;
    }
}

std::string MultiLanguageAnalyzer::getLanguageString(language_detector::SourceLanguage language) const {
    return language_detector::LanguageDetector::getLanguageName(language);
}

} // namespace analyzer
} // namespace core
} // namespace dlogcover 