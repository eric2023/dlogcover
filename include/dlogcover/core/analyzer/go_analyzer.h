/**
 * @file go_analyzer.h
 * @brief Go语言分析器 - 通过外部工具集成实现Go语言AST分析
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_ANALYZER_GO_ANALYZER_H
#define DLOGCOVER_CORE_ANALYZER_GO_ANALYZER_H

#include <dlogcover/core/analyzer/i_language_analyzer.h>
#include <dlogcover/config/config.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <ctime>

namespace dlogcover {
namespace core {
namespace analyzer {

/**
 * @brief Go语言分析器类
 * 
 * 通过调用外部Go分析器工具实现Go语言的AST分析和日志函数识别
 * 注意：这是阶段1的占位符实现，完整功能将在阶段2实现
 */
class GoAnalyzer : public ILanguageAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     */
    explicit GoAnalyzer(const config::Config& config);

    /**
     * @brief 析构函数
     */
    ~GoAnalyzer() override;

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
     * @brief 设置并行模式
     * @param enabled 是否启用并行模式
     * @param maxThreads 最大线程数，0表示自动检测
     */
    void setParallelMode(bool enabled, size_t maxThreads = 0);

    /**
     * @brief 启用或禁用缓存
     * @param enabled 是否启用缓存
     * @param maxCacheSize 最大缓存条目数
     * @param maxMemoryMB 最大内存限制(MB)
     */
    void enableCache(bool enabled, size_t maxCacheSize, size_t maxMemoryMB);

    /**
     * @brief 获取缓存统计信息
     * @return 缓存统计信息字符串
     */
    std::string getCacheStatistics() const;

    /**
     * @brief 清空缓存
     */
    void clearCache();

    /**
     * @brief 批量分析多个Go文件
     * @param filePaths 要分析的文件路径列表
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFiles(const std::vector<std::string>& filePaths);

private:
    /// 配置对象引用
    const config::Config& config_;
    
    /// 分析结果存储
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> results_;
    
    /// Go分析器工具路径
    std::string goAnalyzerPath_;
    
    /// 并行模式配置
    bool parallelEnabled_ = false;
    size_t maxThreads_ = 1;
    
    /// 分析统计信息
    struct Statistics {
        size_t analyzedFiles = 0;
        size_t totalFunctions = 0;
        size_t totalLogCalls = 0;
    } statistics_;

    /// 缓存条目结构
    struct CacheEntry {
        std::string fileHash;           // 文件内容哈希值
        std::time_t lastModified;       // 文件最后修改时间
        std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> results;  // 缓存的分析结果
        std::time_t accessTime;         // 最后访问时间（用于LRU）
        size_t memorySize;              // 估算的内存占用大小
    };
    
    /// 缓存配置和状态
    mutable std::unordered_map<std::string, CacheEntry> cache_;
    bool cacheEnabled_ = false;
    size_t maxCacheSize_ = 100;
    size_t maxMemoryMB_ = 256;
    mutable size_t cacheHits_ = 0;
    mutable size_t cacheMisses_ = 0;
    mutable size_t currentMemoryUsage_ = 0;

    /**
     * @brief 查找Go分析器工具
     * @return Go分析器工具路径
     */
    std::string findGoAnalyzerTool();

    /**
     * @brief 执行外部Go分析器
     * @param filePath 要分析的文件路径
     * @return 分析结果JSON字符串
     */
    ast_analyzer::Result<std::string> executeGoAnalyzer(const std::string& filePath);

    /**
     * @brief 生成Go分析器配置文件
     * @return 配置文件路径
     */
    std::string generateGoConfig();

    /**
     * @brief 解析Go分析器结果
     * @param jsonResult JSON格式的分析结果
     * @return 解析后的AST节点信息列表
     */
    ast_analyzer::Result<std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>> 
    parseGoAnalysisResult(const std::string& jsonResult);

    /**
     * @brief 执行系统命令
     * @param command 要执行的命令
     * @return 命令输出结果
     */
    ast_analyzer::Result<std::string> executeCommand(const std::string& command);

    /**
     * @brief 串行分析多个Go文件
     * @param filePaths 要分析的文件路径列表
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFilesSerial(const std::vector<std::string>& filePaths);

    /**
     * @brief 并行分析多个Go文件
     * @param filePaths 要分析的文件路径列表
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFilesParallel(const std::vector<std::string>& filePaths);

    /**
     * @brief 生成批量分析配置文件
     * @param filePaths 要分析的文件路径列表
     * @param numThreads 使用的线程数量
     * @return 配置文件路径
     */
    std::string generateBatchAnalysisConfig(const std::vector<std::string>& filePaths, size_t numThreads);

    /**
     * @brief 解析批量分析结果
     * @param jsonResult JSON格式的批量分析结果
     * @return 解析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> parseBatchAnalysisResult(const std::string& jsonResult);

    /**
     * @brief 计算文件哈希值
     * @param filePath 文件路径
     * @return 文件哈希值字符串
     */
    std::string calculateFileHash(const std::string& filePath) const;

    /**
     * @brief 获取文件修改时间
     * @param filePath 文件路径
     * @return 文件修改时间
     */
    std::time_t getFileModifiedTime(const std::string& filePath) const;

    /**
     * @brief 检查缓存中是否有有效条目
     * @param filePath 文件路径
     * @return 如果有有效缓存返回true
     */
    bool isCacheValid(const std::string& filePath) const;

    /**
     * @brief 从缓存中获取分析结果
     * @param filePath 文件路径
     * @return 缓存的分析结果，如果不存在返回空向量
     */
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> getCacheResult(const std::string& filePath) const;

    /**
     * @brief 将分析结果添加到缓存
     * @param filePath 文件路径
     * @param results 分析结果
     */
    void addToCache(const std::string& filePath, 
                   const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& results) const;

    /**
     * @brief 清理LRU缓存（当缓存满时）
     */
    void evictLRUCache() const;

    /**
     * @brief 估算分析结果的内存占用
     * @param results 分析结果
     * @return 估算的内存大小（字节）
     */
    size_t estimateMemoryUsage(const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& results) const;
};

} // namespace analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_ANALYZER_GO_ANALYZER_H 