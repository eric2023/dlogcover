/**
 * @file ast_cache.h
 * @brief AST缓存管理器 - 智能缓存AST解析结果
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_CACHE_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_CACHE_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST缓存条目
 */
struct ASTCacheEntry {
    std::string filePath;                                    ///< 文件路径
    std::chrono::system_clock::time_point lastModified;     ///< 最后修改时间
    size_t fileSize;                                         ///< 文件大小
    std::string contentHash;                                 ///< 文件内容哈希
    std::unique_ptr<ASTNodeInfo> astInfo;                    ///< AST信息
    std::chrono::system_clock::time_point cacheTime;        ///< 缓存时间
    size_t accessCount;                                      ///< 访问次数
    std::chrono::system_clock::time_point lastAccessTime;   ///< 最后访问时间
    
    /**
     * @brief 默认构造函数
     */
    ASTCacheEntry() : fileSize(0), accessCount(0) {}
    
    /**
     * @brief 构造函数
     * @param path 文件路径
     * @param modTime 修改时间
     * @param size 文件大小
     * @param hash 内容哈希
     * @param ast AST信息
     */
    ASTCacheEntry(const std::string& path, 
                  const std::chrono::system_clock::time_point& modTime,
                  size_t size, 
                  const std::string& hash,
                  std::unique_ptr<ASTNodeInfo> ast)
        : filePath(path)
        , lastModified(modTime)
        , fileSize(size)
        , contentHash(hash)
        , astInfo(std::move(ast))
        , cacheTime(std::chrono::system_clock::now())
        , accessCount(1)
        , lastAccessTime(std::chrono::system_clock::now()) {}
};

/**
 * @brief AST缓存管理器类
 * 
 * 负责管理AST解析结果的缓存，避免重复解析相同文件
 */
class ASTCache {
public:
    /**
     * @brief 构造函数
     * @param maxSize 最大缓存条目数
     * @param maxMemoryMB 最大内存使用量（MB）
     */
    explicit ASTCache(size_t maxSize = 100, size_t maxMemoryMB = 512);

    /**
     * @brief 析构函数
     */
    ~ASTCache();

    /**
     * @brief 检查缓存是否有效
     * @param filePath 文件路径
     * @return 如果缓存有效返回true
     */
    bool isCacheValid(const std::string& filePath);

    /**
     * @brief 获取缓存的AST信息
     * @param filePath 文件路径
     * @return AST信息，如果不存在返回nullptr
     */
    std::unique_ptr<ASTNodeInfo> getCachedAST(const std::string& filePath);

    /**
     * @brief 缓存AST信息
     * @param filePath 文件路径
     * @param astInfo AST信息
     */
    void cacheAST(const std::string& filePath, std::unique_ptr<ASTNodeInfo> astInfo);

    /**
     * @brief 清空缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存命中次数
     * @return 缓存命中次数
     */
    size_t getCacheHitCount() const { return cacheHits_.load(); }

    /**
     * @brief 获取缓存未命中次数
     * @return 缓存未命中次数
     */
    size_t getCacheMissCount() const { return cacheMisses_.load(); }

    /**
     * @brief 获取缓存命中率
     * @return 缓存命中率（0.0-1.0）
     */
    double getCacheHitRate() const;

    /**
     * @brief 获取当前缓存大小
     * @return 缓存条目数
     */
    size_t getCurrentSize() const;

    /**
     * @brief 获取缓存统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const;

    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试模式
     */
    void setDebugMode(bool enabled) { debugMode_ = enabled; }

    /**
     * @brief 获取估计的内存使用量
     * @return 内存使用量（字节）
     */
    size_t getEstimatedMemoryUsage() const;

private:
    std::unordered_map<std::string, ASTCacheEntry> cache_;  ///< 缓存映射表
    mutable std::mutex cacheMutex_;                         ///< 缓存互斥锁
    size_t maxCacheSize_;                                   ///< 最大缓存大小
    size_t maxMemoryBytes_;                                 ///< 最大内存使用量（字节）
    std::atomic<size_t> cacheHits_{0};                      ///< 缓存命中次数
    std::atomic<size_t> cacheMisses_{0};                    ///< 缓存未命中次数
    bool debugMode_ = false;                                ///< 调试模式

    /**
     * @brief 计算文件内容哈希
     * @param content 文件内容
     * @return 哈希值
     */
    std::string calculateContentHash(const std::string& content);

    /**
     * @brief 检查文件是否发生变化
     * @param filePath 文件路径
     * @param entry 缓存条目
     * @return 如果文件发生变化返回true
     */
    bool hasFileChanged(const std::string& filePath, const ASTCacheEntry& entry);

    /**
     * @brief 执行LRU淘汰
     */
    void evictLRU();

    /**
     * @brief 执行内存压力淘汰
     */
    void evictByMemoryPressure();

    /**
     * @brief 估计AST节点的内存使用量
     * @param astInfo AST节点信息
     * @return 估计的内存使用量（字节）
     */
    size_t estimateASTMemoryUsage(const std::unique_ptr<ASTNodeInfo>& astInfo) const;

    /**
     * @brief 更新访问统计
     * @param entry 缓存条目
     */
    void updateAccessStats(ASTCacheEntry& entry);

    /**
     * @brief 记录调试信息
     * @param message 调试消息
     */
    void debugLog(const std::string& message) const;
};

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_CACHE_H 