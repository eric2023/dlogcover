/**
 * @file cpp_analyzer_adapter.h
 * @brief C++分析器适配器 - 包装现有ASTAnalyzer以实现ILanguageAnalyzer接口
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_ANALYZER_CPP_ANALYZER_ADAPTER_H
#define DLOGCOVER_CORE_ANALYZER_CPP_ANALYZER_ADAPTER_H

#include <dlogcover/core/analyzer/i_language_analyzer.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>

#include <memory>

namespace dlogcover {
namespace config {
class ConfigManager;
} // namespace config

namespace core {
namespace analyzer {

/**
 * @brief C++分析器适配器类
 * 
 * 包装现有的ASTAnalyzer类，使其实现ILanguageAnalyzer接口，
 * 确保现有C++分析功能完全不受影响
 */
class CppAnalyzerAdapter : public ILanguageAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     * @param sourceManager 源文件管理器引用
     * @param configManager 配置管理器引用
     */
    CppAnalyzerAdapter(const config::Config& config, 
                      const source_manager::SourceManager& sourceManager,
                      config::ConfigManager& configManager);

    /**
     * @brief 析构函数
     */
    ~CppAnalyzerAdapter() override;

    /**
     * @brief 分析指定文件
     * @param filePath 要分析的文件路径
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyze(const std::string& filePath) override;

    /**
     * @brief 获取分析结果
     * @return AST节点信息列表的常量引用
     */
    const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& getResults() const override;

    /**
     * @brief 清空分析结果
     */
    void clear() override;

    /**
     * @brief 获取分析器支持的语言名称
     * @return 语言名称字符串
     */
    std::string getLanguageName() const override;

    /**
     * @brief 检查分析器是否已启用
     * @return 如果启用返回true，否则返回false
     */
    bool isEnabled() const override;

    /**
     * @brief 获取支持的文件扩展名列表
     * @return 文件扩展名列表
     */
    std::vector<std::string> getSupportedExtensions() const override;

    /**
     * @brief 获取分析统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const override;

    /**
     * @brief 分析所有文件
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeAll();

    /**
     * @brief 并行分析所有文件
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeAllParallel();

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
     * @brief 获取缓存统计信息
     * @return 缓存统计信息字符串
     */
    std::string getCacheStatistics() const;

    /**
     * @brief 获取指定文件的AST节点信息
     * @param filePath 文件路径
     * @return AST节点信息指针，如果不存在返回nullptr
     */
    const ast_analyzer::ASTNodeInfo* getASTNodeInfo(const std::string& filePath) const;

    /**
     * @brief 获取所有文件的AST节点信息
     * @return 所有文件的AST节点信息映射表的常量引用
     */
    const std::unordered_map<std::string, std::unique_ptr<ast_analyzer::ASTNodeInfo>>& getAllASTNodeInfo() const;

    /**
     * @brief 获取底层的AST分析器实例
     * @return AST分析器指针
     * @note 用于直接访问原有的C++分析功能，保持性能优势
     */
    ast_analyzer::ASTAnalyzer* getUnderlyingAnalyzer() const;

private:
    /// 包装的AST分析器实例
    std::unique_ptr<ast_analyzer::ASTAnalyzer> astAnalyzer_;
    
    /// 配置对象引用
    const config::Config& config_;
};

} // namespace analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_ANALYZER_CPP_ANALYZER_ADAPTER_H 