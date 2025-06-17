/**
 * @file cpp_analyzer_adapter.cpp
 * @brief C++分析器适配器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/analyzer/cpp_analyzer_adapter.h>
#include <dlogcover/core/language_detector/language_detector.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace analyzer {

CppAnalyzerAdapter::CppAnalyzerAdapter(const config::Config& config, 
                                      const source_manager::SourceManager& sourceManager,
                                      config::ConfigManager& configManager)
    : config_(config) {
    LOG_DEBUG("初始化C++分析器适配器");
    
    // 创建包装的AST分析器实例
    astAnalyzer_ = std::make_unique<ast_analyzer::ASTAnalyzer>(config, sourceManager, configManager);
    
    LOG_INFO("C++分析器适配器初始化完成");
}

CppAnalyzerAdapter::~CppAnalyzerAdapter() {
    LOG_DEBUG("销毁C++分析器适配器");
}

ast_analyzer::Result<bool> CppAnalyzerAdapter::analyze(const std::string& filePath) {
    LOG_DEBUG_FMT("C++分析器适配器分析文件: %s", filePath.c_str());
    
    // 检查文件是否为C++文件
    auto language = language_detector::LanguageDetector::detectLanguage(filePath);
    if (language != language_detector::SourceLanguage::CPP) {
        LOG_WARNING_FMT("文件不是C++文件，跳过分析: %s", filePath.c_str());
        return ast_analyzer::makeSuccess(true);
    }
    
    // 委托给包装的AST分析器
    return astAnalyzer_->analyze(filePath);
}

const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& CppAnalyzerAdapter::getResults() const {
    // 注意：在多语言架构下，C++分析结果存储在AST节点映射中，而不是results_向量中
    // 这里我们需要从AST节点映射中构建results_向量
    static std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> cachedResults;
    
    // 清空之前的缓存结果
    cachedResults.clear();
    
    // 从AST节点映射中提取所有结果
    const auto& allASTNodes = astAnalyzer_->getAllASTNodeInfo();
    for (const auto& [filePath, nodeInfo] : allASTNodes) {
        if (nodeInfo) {
            // 创建节点信息的副本
            auto resultCopy = std::make_unique<ast_analyzer::ASTNodeInfo>(*nodeInfo);
            cachedResults.push_back(std::move(resultCopy));
        }
    }
    
    LOG_DEBUG_FMT("C++分析器适配器从AST节点映射中提取了 %zu 个结果", cachedResults.size());
    
    return cachedResults;
}

void CppAnalyzerAdapter::clear() {
    LOG_DEBUG("清空C++分析器适配器结果");
    astAnalyzer_->clear();
}

std::string CppAnalyzerAdapter::getLanguageName() const {
    return "C++";
}

bool CppAnalyzerAdapter::isEnabled() const {
    // C++分析器始终启用（保持现有行为）
    return true;
}

std::vector<std::string> CppAnalyzerAdapter::getSupportedExtensions() const {
    return {".cpp", ".cxx", ".cc", ".c++", ".C", ".h", ".hpp", ".hxx", ".h++", ".hh"};
}

std::string CppAnalyzerAdapter::getStatistics() const {
    // 获取所有AST节点信息（包括外部添加的结果）
    const auto& allASTNodes = astAnalyzer_->getAllASTNodeInfo();
    size_t totalFiles = allASTNodes.size();
    size_t totalNodes = 0;
    size_t totalLogNodes = 0;
    
    // 统计所有AST节点的信息
    for (const auto& [filePath, nodeInfo] : allASTNodes) {
        if (nodeInfo) {
            totalNodes++;
            if (nodeInfo->hasLogging) {
                totalLogNodes++;
            }
            
            // 递归统计子节点
            std::function<void(const ast_analyzer::ASTNodeInfo*)> countNodes = 
                [&](const ast_analyzer::ASTNodeInfo* node) {
                    if (!node) return;
                    for (const auto& child : node->children) {
                        if (child) {
                            totalNodes++;
                            if (child->hasLogging) {
                                totalLogNodes++;
                            }
                            countNodes(child.get());
                        }
                    }
                };
            countNodes(nodeInfo.get());
        }
    }
    
    return "C++分析统计: " + std::to_string(totalFiles) + " 个文件, " +
           std::to_string(totalNodes) + " 个节点, " +
           std::to_string(totalLogNodes) + " 个包含日志的节点";
}

ast_analyzer::Result<bool> CppAnalyzerAdapter::analyzeAll() {
    LOG_INFO("C++分析器适配器开始分析所有文件");
    return astAnalyzer_->analyzeAll();
}

ast_analyzer::Result<bool> CppAnalyzerAdapter::analyzeAllParallel() {
    LOG_INFO("C++分析器适配器开始并行分析所有文件");
    return astAnalyzer_->analyzeAllParallel();
}

void CppAnalyzerAdapter::setParallelMode(bool enabled, size_t maxThreads) {
    LOG_DEBUG_FMT("设置C++分析器并行模式: enabled=%s, maxThreads=%zu", 
                  enabled ? "true" : "false", maxThreads);
    astAnalyzer_->setParallelMode(enabled, maxThreads);
}

void CppAnalyzerAdapter::enableCache(bool enabled, size_t maxCacheSize, size_t maxMemoryMB) {
    LOG_DEBUG_FMT("设置C++分析器缓存: enabled=%s, maxCacheSize=%zu, maxMemoryMB=%zu", 
                  enabled ? "true" : "false", maxCacheSize, maxMemoryMB);
    astAnalyzer_->enableCache(enabled, maxCacheSize, maxMemoryMB);
}

std::string CppAnalyzerAdapter::getCacheStatistics() const {
    return astAnalyzer_->getCacheStatistics();
}

const ast_analyzer::ASTNodeInfo* CppAnalyzerAdapter::getASTNodeInfo(const std::string& filePath) const {
    return astAnalyzer_->getASTNodeInfo(filePath);
}

const std::unordered_map<std::string, std::unique_ptr<ast_analyzer::ASTNodeInfo>>& 
CppAnalyzerAdapter::getAllASTNodeInfo() const {
    return astAnalyzer_->getAllASTNodeInfo();
}

ast_analyzer::ASTAnalyzer* CppAnalyzerAdapter::getUnderlyingAnalyzer() const {
    return astAnalyzer_.get();
}

} // namespace analyzer
} // namespace core
} // namespace dlogcover 