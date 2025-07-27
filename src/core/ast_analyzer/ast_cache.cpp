/**
 * @file ast_cache.cpp
 * @brief AST缓存管理器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_cache.h>
#include <dlogcover/utils/log_utils.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

ASTCache::ASTCache(size_t maxSize, size_t maxMemoryMB) 
    : maxCacheSize_(maxSize)
    , maxMemoryBytes_(maxMemoryMB * 1024 * 1024) {
    LOG_INFO_FMT("初始化AST缓存，最大条目数: %zu, 最大内存: %zu MB", 
                maxSize, maxMemoryMB);
}

ASTCache::~ASTCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    if (debugMode_) {
        LOG_INFO("AST缓存析构，最终统计信息:");
        LOG_INFO(getStatistics().c_str());
    }
    
    cache_.clear();
}

bool ASTCache::isCacheValid(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    auto it = cache_.find(filePath);
    if (it == cache_.end()) {
        cacheMisses_.fetch_add(1);
        debugLog("缓存未命中: " + filePath);
        return false;
    }
    
    // 检查文件是否发生变化
    if (hasFileChanged(filePath, it->second)) {
        debugLog("文件已变化，移除缓存: " + filePath);
        cache_.erase(it);
        cacheMisses_.fetch_add(1);
        return false;
    }
    
    // 检查依赖文件是否发生变化
    if (hasDependenciesChanged(it->second)) {
        debugLog("依赖文件已变化，移除缓存: " + filePath);
        cache_.erase(it);
        cacheMisses_.fetch_add(1);
        return false;
    }
    
    // 更新访问统计
    updateAccessStats(it->second);
    cacheHits_.fetch_add(1);
    debugLog("缓存命中: " + filePath);
    return true;
}

std::unique_ptr<ASTNodeInfo> ASTCache::getCachedAST(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    auto it = cache_.find(filePath);
    if (it == cache_.end()) {
        return nullptr;
    }
    
    // 再次检查文件是否发生变化
    if (hasFileChanged(filePath, it->second)) {
        cache_.erase(it);
        return nullptr;
    }
    
    // 检查依赖文件是否发生变化
    if (hasDependenciesChanged(it->second)) {
        cache_.erase(it);
        return nullptr;
    }
    
    // 更新访问统计
    updateAccessStats(it->second);
    
    debugLog("返回缓存的AST: " + filePath);
    
    // 创建深拷贝返回
    if (it->second.astInfo) {
        return std::make_unique<ASTNodeInfo>(*it->second.astInfo);
    }
    return nullptr;
}

void ASTCache::cacheAST(const std::string& filePath, std::unique_ptr<ASTNodeInfo> astInfo,
                        const std::vector<std::string>& dependencies) {
    if (!astInfo) {
        LOG_WARNING_FMT("尝试缓存空的AST信息: %s", filePath.c_str());
        return;
    }
    
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    try {
        // 获取文件信息
        auto lastModified = std::filesystem::last_write_time(filePath);
        auto fileSize = std::filesystem::file_size(filePath);
        
        // 如果没有提供依赖关系，则自动扫描
        std::vector<std::string> actualDependencies = dependencies;
        if (actualDependencies.empty()) {
            actualDependencies = scanFileDependencies(filePath);
        }
        
        // 读取文件内容计算哈希
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_WARNING_FMT("无法打开文件计算哈希: %s", filePath.c_str());
            return;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        std::string contentHash = calculateContentHash(content);
        
        // 检查是否需要淘汰
        if (cache_.size() >= maxCacheSize_) {
            evictLRU();
        }
        
        // 检查内存压力
        size_t estimatedSize = estimateASTMemoryUsage(astInfo);
        if (getEstimatedMemoryUsage() + estimatedSize > maxMemoryBytes_) {
            evictByMemoryPressure();
        }
        
        // 创建缓存条目
        ASTCacheEntry entry(filePath, lastModified, fileSize, contentHash, std::move(astInfo));
        entry.dependencies = actualDependencies;
        entry.dependenciesLastCheck = std::filesystem::file_time_type::clock::now();
        cache_[filePath] = std::move(entry);
        
        debugLog("缓存AST信息: " + filePath + 
                ", 估计大小: " + std::to_string(estimatedSize) + " 字节");
        
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("缓存AST信息失败: %s, 错误: %s", filePath.c_str(), e.what());
    }
}

void ASTCache::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    size_t oldSize = cache_.size();
    cache_.clear();
    cacheHits_.store(0);
    cacheMisses_.store(0);
    
    LOG_INFO_FMT("清空AST缓存，移除了 %zu 个条目", oldSize);
}

double ASTCache::getCacheHitRate() const {
    size_t hits = cacheHits_.load();
    size_t misses = cacheMisses_.load();
    size_t total = hits + misses;
    
    if (total == 0) {
        return 0.0;
    }
    
    return static_cast<double>(hits) / static_cast<double>(total);
}

size_t ASTCache::getCurrentSize() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    return cache_.size();
}

std::string ASTCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    size_t hits = cacheHits_.load();
    size_t misses = cacheMisses_.load();
    size_t total = hits + misses;
    double hitRate = getCacheHitRate();
    size_t memoryUsage = getEstimatedMemoryUsage();
    
    oss << "AST缓存统计信息:\n";
    oss << "  缓存条目数: " << cache_.size() << "/" << maxCacheSize_ << "\n";
    oss << "  缓存命中: " << hits << "\n";
    oss << "  缓存未命中: " << misses << "\n";
    oss << "  总访问次数: " << total << "\n";
    oss << "  命中率: " << (hitRate * 100.0) << "%\n";
    oss << "  内存使用: " << (memoryUsage / 1024.0 / 1024.0) << " MB / " 
        << (maxMemoryBytes_ / 1024.0 / 1024.0) << " MB\n";
    
    if (!cache_.empty()) {
        // 计算平均访问次数
        size_t totalAccess = 0;
        for (const auto& pair : cache_) {
            totalAccess += pair.second.accessCount;
        }
        double avgAccess = static_cast<double>(totalAccess) / cache_.size();
        oss << "  平均访问次数: " << avgAccess << "\n";
    }
    
    return oss.str();
}

size_t ASTCache::getEstimatedMemoryUsage() const {
    size_t totalMemory = 0;
    
    for (const auto& pair : cache_) {
        totalMemory += estimateASTMemoryUsage(pair.second.astInfo);
        totalMemory += pair.first.size(); // 文件路径
        totalMemory += pair.second.contentHash.size(); // 哈希值
        totalMemory += sizeof(ASTCacheEntry); // 结构体本身
    }
    
    return totalMemory;
}

std::string ASTCache::calculateContentHash(const std::string& content) {
    // 使用简单但快速的哈希算法
    std::hash<std::string> hasher;
    size_t hashValue = hasher(content);
    
    // 转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << hashValue;
    return oss.str();
}

bool ASTCache::hasFileChanged(const std::string& filePath, const ASTCacheEntry& entry) {
    try {
        auto currentModTime = std::filesystem::last_write_time(filePath);
        auto currentSize = std::filesystem::file_size(filePath);
        
        // 直接比较文件系统时间类型和文件大小
        if (currentModTime != entry.lastModified || currentSize != entry.fileSize) {
            debugLog("文件已变化: " + filePath + 
                    ", 时间或大小不匹配");
            return true;
        }
        
        // 对于重要文件，还可以检查内容哈希
        // 这里为了性能考虑，暂时只检查时间和大小
        
        return false;
    } catch (const std::exception& e) {
        debugLog("检查文件变化时发生错误: " + filePath + ", " + e.what());
        return true; // 出错时认为文件已变化，安全起见
    }
}

void ASTCache::evictLRU() {
    if (cache_.empty()) {
        return;
    }
    
    // 找到最久未访问的条目
    auto oldestIt = cache_.begin();
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.lastAccessTime < oldestIt->second.lastAccessTime) {
            oldestIt = it;
        }
    }
    
    debugLog("LRU淘汰缓存条目: " + oldestIt->first);
    cache_.erase(oldestIt);
}

void ASTCache::evictByMemoryPressure() {
    if (cache_.empty()) {
        return;
    }
    
    // 按内存使用量排序，优先淘汰占用内存大的条目
    std::vector<std::pair<std::string, size_t>> memoryUsage;
    
    for (const auto& pair : cache_) {
        size_t usage = estimateASTMemoryUsage(pair.second.astInfo);
        memoryUsage.emplace_back(pair.first, usage);
    }
    
    // 按内存使用量降序排序
    std::sort(memoryUsage.begin(), memoryUsage.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    // 淘汰占用内存最大的条目
    if (!memoryUsage.empty()) {
        const std::string& pathToEvict = memoryUsage[0].first;
        debugLog("内存压力淘汰缓存条目: " + pathToEvict + 
                ", 内存使用: " + std::to_string(memoryUsage[0].second) + " 字节");
        cache_.erase(pathToEvict);
    }
}

size_t ASTCache::estimateASTMemoryUsage(const std::unique_ptr<ASTNodeInfo>& astInfo) const {
    if (!astInfo) {
        return 0;
    }
    
    // 估算AST节点的内存使用量
    size_t baseSize = sizeof(ASTNodeInfo);
    size_t textSize = astInfo->text.size();
    size_t nameSize = astInfo->name.size();
    size_t childrenSize = astInfo->children.size() * sizeof(std::unique_ptr<ASTNodeInfo>);
    
    // 递归计算子节点
    for (const auto& child : astInfo->children) {
        childrenSize += estimateASTMemoryUsage(child);
    }
    
    return baseSize + textSize + nameSize + childrenSize;
}

void ASTCache::updateAccessStats(ASTCacheEntry& entry) {
    entry.accessCount++;
    entry.lastAccessTime = std::chrono::system_clock::now();
}

void ASTCache::debugLog(const std::string& message) const {
    if (debugMode_) {
        LOG_DEBUG_FMT("[ASTCache] %s", message.c_str());
    }
}

bool ASTCache::hasDependenciesChanged(const ASTCacheEntry& entry) {
    // 如果没有依赖关系，则认为未变化
    if (entry.dependencies.empty()) {
        return false;
    }
    
    try {
        for (const auto& depPath : entry.dependencies) {
            if (!std::filesystem::exists(depPath)) {
                debugLog("依赖文件不存在: " + depPath);
                return true;
            }
            
            auto lastModified = std::filesystem::last_write_time(depPath);
            if (lastModified > entry.dependenciesLastCheck) {
                debugLog("依赖文件已修改: " + depPath);
                return true;
            }
        }
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("检查依赖关系时出错: %s", e.what());
        return true;  // 出错时保守地认为依赖已变化
    }
    
    return false;
}

std::vector<std::string> ASTCache::scanFileDependencies(const std::string& filePath) {
    std::vector<std::string> dependencies;
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return dependencies;
        }
        
        std::string line;
        std::filesystem::path basePath = std::filesystem::path(filePath).parent_path();
        
        while (std::getline(file, line)) {
            // 简单的#include解析
            if (line.find("#include") != std::string::npos) {
                // 查找引号或尖括号内的文件名
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                
                if (start == std::string::npos) {
                    start = line.find('<');
                    end = line.find('>', start + 1);
                }
                
                if (start != std::string::npos && end != std::string::npos && end > start) {
                    std::string includePath = line.substr(start + 1, end - start - 1);
                    
                    // 尝试解析相对路径
                    if (includePath[0] != '/') {
                        std::filesystem::path fullPath = basePath / includePath;
                        if (std::filesystem::exists(fullPath)) {
                            dependencies.push_back(fullPath.string());
                        }
                    } else if (std::filesystem::exists(includePath)) {
                        dependencies.push_back(includePath);
                    }
                }
            }
        }
        
        debugLog("扫描到 " + std::to_string(dependencies.size()) + " 个依赖文件: " + filePath);
        
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("扫描文件依赖关系失败: %s, 错误: %s", filePath.c_str(), e.what());
    }
    
    return dependencies;
}

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover 