/**
 * @file multi_language_analyzer.h
 * @brief 多语言分析器框架 - 统一管理不同语言的分析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_ANALYZER_MULTI_LANGUAGE_ANALYZER_H
#define DLOGCOVER_CORE_ANALYZER_MULTI_LANGUAGE_ANALYZER_H

#include <dlogcover/core/analyzer/i_language_analyzer.h>
#include <dlogcover/core/language_detector/language_detector.h>
#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace dlogcover {
namespace config {
class ConfigManager;
} // namespace config

namespace core {
namespace analyzer {

/**
 * @brief 多语言分析器框架类
 * 
 * 统一管理不同语言的分析器，根据配置的分析模式选择最优的分析策略：
 * - CPP_ONLY: 纯C++项目，保持原有性能优势
 * - GO_ONLY: 纯Go项目，启用多线程优化
 * - AUTO_DETECT: 混合项目，支持C++和Go自动检测
 */
class MultiLanguageAnalyzer {
public:
    /**
     * @brief 分析模式枚举
     */
    enum class AnalysisMode {
        CPP_ONLY,      ///< 纯C++项目模式 - 走原有实现，保持性能优势
        GO_ONLY,       ///< 纯Go项目模式 - 完善多线程支持
        AUTO_DETECT    ///< 自动检测模式 - 支持C++和Go混合项目
    };

    /**
     * @brief 构造函数
     * @param config 配置对象引用
     * @param sourceManager 源文件管理器引用
     * @param configManager 配置管理器引用
     */
    MultiLanguageAnalyzer(const config::Config& config, 
                         const source_manager::SourceManager& sourceManager,
                         config::ConfigManager& configManager);

    /**
     * @brief 析构函数
     */
    ~MultiLanguageAnalyzer();

    /**
     * @brief 分析指定文件
     * @param filePath 要分析的文件路径
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFile(const std::string& filePath);

    /**
     * @brief 分析所有文件 - 根据分析模式和并行配置选择最优策略
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeAll();

    /**
     * @brief 获取合并后的所有分析结果
     * @return 所有语言的AST节点信息列表
     */
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> getAllResults() const;

    /**
     * @brief 清空所有分析结果
     */
    void clearAll();

    /**
     * @brief 获取指定语言的分析器
     * @param language 语言类型
     * @return 分析器指针，如果不存在返回nullptr
     */
    ILanguageAnalyzer* getAnalyzer(language_detector::SourceLanguage language) const;

    /**
     * @brief 获取C++分析器
     * @return C++分析器指针
     */
    ILanguageAnalyzer* getCppAnalyzer() const;

    /**
     * @brief 获取Go分析器
     * @return Go分析器指针，如果未启用返回nullptr
     */
    ILanguageAnalyzer* getGoAnalyzer() const;

    /**
     * @brief 获取支持的语言列表
     * @return 支持的语言类型列表
     */
    std::vector<language_detector::SourceLanguage> getSupportedLanguages() const;

    /**
     * @brief 获取分析统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const;

    /**
     * @brief 设置并行模式
     * @param enabled 是否启用并行模式
     * @param maxThreads 最大线程数，0表示自动检测
     */
    void setParallelMode(bool enabled, size_t maxThreads = 0);

    /**
     * @brief 启用AST缓存
     * @param enabled 是否启用缓存
     * @param maxCacheSize 最大缓存大小
     * @param maxMemoryMB 最大内存使用量（MB）
     */
    void enableCache(bool enabled, size_t maxCacheSize = 100, size_t maxMemoryMB = 512);

    /**
     * @brief 获取所有语言分析器的缓存统计信息
     * @return 详细的缓存统计信息字符串
     */
    std::string getAllCacheStatistics() const;

    /**
     * @brief 清空所有分析器的缓存
     */
    void clearAllCache();

    /**
     * @brief 获取总缓存大小（所有分析器）
     * @return 总缓存条目数
     */
    size_t getTotalCacheSize() const;

    /**
     * @brief 检查指定语言是否已启用
     * @param language 语言类型
     * @return 如果启用返回true，否则返回false
     */
    bool isLanguageEnabled(language_detector::SourceLanguage language) const;

    /**
     * @brief 获取当前分析模式
     * @return 当前的分析模式
     */
    AnalysisMode getAnalysisMode() const { return analysisMode_; }

    /**
     * @brief 获取分析模式的字符串表示
     * @param mode 分析模式
     * @return 模式名称字符串
     */
    static std::string getAnalysisModeString(AnalysisMode mode);

private:
    /// 配置对象引用
    const config::Config& config_;
    
    /// 源文件管理器引用
    const source_manager::SourceManager& sourceManager_;
    
    /// 配置管理器引用
    config::ConfigManager& configManager_;
    
    /// 当前分析模式
    AnalysisMode analysisMode_;
    
    /// 检测到的项目主要语言（用于AUTO_DETECT模式）
    language_detector::SourceLanguage detectedLanguage_;
    
    /// 语言分析器映射表
    std::unordered_map<language_detector::SourceLanguage, std::unique_ptr<ILanguageAnalyzer>> analyzers_;
    
    /// 分析统计信息
    struct AnalysisStatistics {
        size_t totalFiles = 0;
        size_t cppFiles = 0;
        size_t goFiles = 0;
        size_t unknownFiles = 0;
        size_t successfulAnalyses = 0;
        size_t failedAnalyses = 0;
    } statistics_;

    /**
     * @brief 根据配置确定分析模式
     */
    void determineAnalysisMode();

    /**
     * @brief 初始化所需的分析器
     */
    void initializeRequiredAnalyzers();

    /**
     * @brief 纯C++分析模式 - 保持原有性能
     * @return 分析结果
     */
    ast_analyzer::Result<bool> analyzeCppOnlyMode();

    /**
     * @brief 纯Go分析模式 - 启用多线程优化
     * @return 分析结果
     */
    ast_analyzer::Result<bool> analyzeGoOnlyMode();

    /**
     * @brief 自动检测分析模式 - 支持混合项目
     * @return 分析结果
     */
    ast_analyzer::Result<bool> analyzeAutoDetectMode();

    /**
     * @brief 自动检测并行分析模式 - 支持混合项目多线程分析
     * @return 分析结果
     */
    ast_analyzer::Result<bool> analyzeAutoDetectModeParallel();

    /**
     * @brief 项目语言检测 - 抽样检测
     */
    void detectProjectLanguage();

    /**
     * @brief 收集指定类型的文件
     * @param language 语言类型
     * @return 文件路径列表
     */
    std::vector<std::string> collectFilesByLanguage(language_detector::SourceLanguage language);

    /**
     * @brief 初始化所有分析器（原有方法，保持兼容）
     */
    void initializeAnalyzers();

    /**
     * @brief 创建C++分析器
     * @return C++分析器实例
     */
    std::unique_ptr<ILanguageAnalyzer> createCppAnalyzer();

    /**
     * @brief 创建Go分析器
     * @return Go分析器实例，如果未启用返回nullptr
     */
    std::unique_ptr<ILanguageAnalyzer> createGoAnalyzer();

    /**
     * @brief 更新分析统计信息
     * @param language 分析的语言类型
     * @param success 分析是否成功
     */
    void updateStatistics(language_detector::SourceLanguage language, bool success);

    /**
     * @brief 获取语言类型的字符串表示
     * @param language 语言类型
     * @return 语言名称字符串
     */
    std::string getLanguageString(language_detector::SourceLanguage language) const;
};

} // namespace analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_ANALYZER_MULTI_LANGUAGE_ANALYZER_H 