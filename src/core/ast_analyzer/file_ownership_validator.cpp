/**
 * @file file_ownership_validator.cpp
 * @brief 文件归属验证引擎实现
 * @author DLogCover Team
 * @date 2024
 */

#include "dlogcover/core/ast_analyzer/file_ownership_validator.h"
#include "dlogcover/utils/log_utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <regex>
#include <chrono>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief FileOwnershipValidator的私有实现
 */
struct FileOwnershipValidator::Impl {
    std::string projectRoot;
    std::vector<std::string> includeDirectories;
    std::vector<std::string> excludePatterns;
    std::unordered_map<std::string, ValidationResult> cache;
    bool cacheEnabled = true;
    bool debugMode = false;
    
    // 统计信息
    mutable size_t totalValidations = 0;
    mutable size_t cacheHits = 0;
    mutable size_t strictMatches = 0;
    mutable size_t canonicalMatches = 0;
    mutable size_t smartMatches = 0;
    mutable size_t fuzzyMatches = 0;
    
    utils::PathNormalizer pathNormalizer;
};

FileOwnershipValidator::FileOwnershipValidator() 
    : pImpl_(std::make_unique<Impl>()) {
}

FileOwnershipValidator::~FileOwnershipValidator() = default;

FileOwnershipValidator::ValidationResult FileOwnershipValidator::validateOwnership(
    const std::string& targetFile,
    const std::string& declFile,
    ValidationLevel level) {
    
    pImpl_->totalValidations++;
    
    // 检查缓存
    if (pImpl_->cacheEnabled) {
        std::string cacheKey = getCacheKey(targetFile, declFile, level);
        auto it = pImpl_->cache.find(cacheKey);
        if (it != pImpl_->cache.end()) {
            pImpl_->cacheHits++;
            if (pImpl_->debugMode) {
                LOG_DEBUG_FMT("[FileOwnershipValidator] Cache hit for: %s", cacheKey.c_str());
            }
            return it->second;
        }
    }
    
    ValidationResult result;
    
    // 根据验证级别选择验证方法
    switch (level) {
        case ValidationLevel::STRICT:
            result = validateStrict(targetFile, declFile);
            break;
        case ValidationLevel::CANONICAL:
            result = validateCanonical(targetFile, declFile);
            break;
        case ValidationLevel::SMART:
            result = validateSmart(targetFile, declFile);
            break;
        case ValidationLevel::FUZZY:
            result = validateFuzzy(targetFile, declFile);
            break;
    }
    
    result.usedLevel = level;
    
    // 更新统计信息
    if (result.isOwned) {
        switch (level) {
            case ValidationLevel::STRICT: pImpl_->strictMatches++; break;
            case ValidationLevel::CANONICAL: pImpl_->canonicalMatches++; break;
            case ValidationLevel::SMART: pImpl_->smartMatches++; break;
            case ValidationLevel::FUZZY: pImpl_->fuzzyMatches++; break;
        }
    }
    
    // 缓存结果
    if (pImpl_->cacheEnabled) {
        std::string cacheKey = getCacheKey(targetFile, declFile, level);
        pImpl_->cache[cacheKey] = result;
    }
    
    if (pImpl_->debugMode) {
        LOG_DEBUG_FMT("[FileOwnershipValidator] Validation result: %s, Level: %s, Confidence: %.2f, Reason: %s",
                      (result.isOwned ? "OWNED" : "NOT_OWNED"),
                      validationLevelToString(level).c_str(),
                      result.confidence,
                      result.reason.c_str());
    }
    
    return result;
}

std::vector<FileOwnershipValidator::ValidationResult> FileOwnershipValidator::validateOwnershipBatch(
    const std::string& targetFile,
    const std::vector<std::string>& declFiles,
    ValidationLevel level) {
    
    std::vector<ValidationResult> results;
    results.reserve(declFiles.size());
    
    for (const auto& declFile : declFiles) {
        results.push_back(validateOwnership(targetFile, declFile, level));
    }
    
    return results;
}

void FileOwnershipValidator::setProjectRoot(const std::string& projectRoot) {
    pImpl_->projectRoot = pImpl_->pathNormalizer.getCanonicalPath(projectRoot);
    clearCache(); // 项目根目录变化时清理缓存
}

void FileOwnershipValidator::addIncludeDirectory(const std::string& includeDir) {
    std::string normalizedDir = pImpl_->pathNormalizer.getCanonicalPath(includeDir);
    pImpl_->includeDirectories.push_back(normalizedDir);
}

void FileOwnershipValidator::addExcludePattern(const std::string& pattern) {
    pImpl_->excludePatterns.push_back(pattern);
}

void FileOwnershipValidator::clearCache() {
    pImpl_->cache.clear();
}

void FileOwnershipValidator::setCacheEnabled(bool enabled) {
    pImpl_->cacheEnabled = enabled;
    if (!enabled) {
        clearCache();
    }
}

void FileOwnershipValidator::setDebugMode(bool enabled) {
    pImpl_->debugMode = enabled;
}

std::string FileOwnershipValidator::getStatistics() const {
    std::ostringstream oss;
    oss << "FileOwnershipValidator Statistics:\n";
    oss << "  Total Validations: " << pImpl_->totalValidations << "\n";
    oss << "  Cache Hits: " << pImpl_->cacheHits << " (" 
        << (pImpl_->totalValidations > 0 ? (pImpl_->cacheHits * 100.0 / pImpl_->totalValidations) : 0.0) 
        << "%)\n";
    oss << "  Strict Matches: " << pImpl_->strictMatches << "\n";
    oss << "  Canonical Matches: " << pImpl_->canonicalMatches << "\n";
    oss << "  Smart Matches: " << pImpl_->smartMatches << "\n";
    oss << "  Fuzzy Matches: " << pImpl_->fuzzyMatches << "\n";
    oss << "  Cache Size: " << pImpl_->cache.size() << "\n";
    return oss.str();
}

FileOwnershipValidator::ValidationResult FileOwnershipValidator::validateStrict(
    const std::string& targetFile, const std::string& declFile) {
    
    ValidationResult result;
    result.normalizedTargetPath = targetFile;
    result.normalizedDeclPath = declFile;
    
    // 严格模式：完全字符串匹配
    result.isOwned = (targetFile == declFile);
    result.confidence = result.isOwned ? 1.0 : 0.0;
    result.reason = result.isOwned ? "Exact string match" : "String mismatch";
    
    return result;
}

FileOwnershipValidator::ValidationResult FileOwnershipValidator::validateCanonical(
    const std::string& targetFile, const std::string& declFile) {
    
    ValidationResult result;
    
    try {
        result.normalizedTargetPath = pImpl_->pathNormalizer.getCanonicalPath(targetFile);
        result.normalizedDeclPath = pImpl_->pathNormalizer.getCanonicalPath(declFile);
        
        result.isOwned = pImpl_->pathNormalizer.isSamePath(result.normalizedTargetPath, result.normalizedDeclPath);
        result.confidence = result.isOwned ? 0.95 : 0.0;
        result.reason = result.isOwned ? "Canonical path match" : "Canonical path mismatch";
        
    } catch (const std::exception& e) {
        result.isOwned = false;
        result.confidence = 0.0;
        result.reason = std::string("Canonical path resolution failed: ") + e.what();
        result.normalizedTargetPath = targetFile;
        result.normalizedDeclPath = declFile;
    }
    
    return result;
}

FileOwnershipValidator::ValidationResult FileOwnershipValidator::validateSmart(
    const std::string& targetFile, const std::string& declFile) {
    
    ValidationResult result;
    
    try {
        result.normalizedTargetPath = pImpl_->pathNormalizer.getCanonicalPath(targetFile);
        result.normalizedDeclPath = pImpl_->pathNormalizer.getCanonicalPath(declFile);
        
        // 1. 首先尝试规范化路径匹配
        if (pImpl_->pathNormalizer.isSamePath(result.normalizedTargetPath, result.normalizedDeclPath)) {
            result.isOwned = true;
            result.confidence = 0.95;
            result.reason = "Canonical path match";
            return result;
        }
        
        // 2. 检查是否匹配排除模式
        if (matchesExcludePattern(result.normalizedDeclPath)) {
            result.isOwned = false;
            result.confidence = 0.9;
            result.reason = "Matches exclude pattern";
            return result;
        }
        
        // 3. 检查是否为头文件对应的源文件
        if (isCorrespondingSourceFile(result.normalizedTargetPath, result.normalizedDeclPath) ||
            isCorrespondingSourceFile(result.normalizedDeclPath, result.normalizedTargetPath)) {
            result.isOwned = true;
            result.confidence = 0.8;
            result.reason = "Corresponding header/source file";
            return result;
        }
        
        // 4. 检查是否在同一目录下
        std::string targetDir = pImpl_->pathNormalizer.getDirectoryPath(result.normalizedTargetPath);
        std::string declDir = pImpl_->pathNormalizer.getDirectoryPath(result.normalizedDeclPath);
        
        if (pImpl_->pathNormalizer.isSamePath(targetDir, declDir)) {
            std::string targetFileName = pImpl_->pathNormalizer.getFileName(result.normalizedTargetPath);
            std::string declFileName = pImpl_->pathNormalizer.getFileName(result.normalizedDeclPath);
            
            if (targetFileName == declFileName) {
                result.isOwned = true;
                result.confidence = 0.7;
                result.reason = "Same directory and filename";
                return result;
            }
        }
        
        // 5. 计算路径相似度
        double similarity = calculatePathSimilarity(result.normalizedTargetPath, result.normalizedDeclPath);
        if (similarity > 0.8) {
            result.isOwned = true;
            result.confidence = similarity * 0.6; // 降低置信度
            result.reason = "High path similarity (" + std::to_string(similarity) + ")";
            return result;
        }
        
        // 6. 默认不匹配
        result.isOwned = false;
        result.confidence = 0.0;
        result.reason = "No smart matching criteria met";
        
    } catch (const std::exception& e) {
        result.isOwned = false;
        result.confidence = 0.0;
        result.reason = std::string("Smart validation failed: ") + e.what();
        result.normalizedTargetPath = targetFile;
        result.normalizedDeclPath = declFile;
    }
    
    return result;
}

FileOwnershipValidator::ValidationResult FileOwnershipValidator::validateFuzzy(
    const std::string& targetFile, const std::string& declFile) {
    
    ValidationResult result;
    result.normalizedTargetPath = pImpl_->pathNormalizer.normalize(targetFile);
    result.normalizedDeclPath = pImpl_->pathNormalizer.normalize(declFile);
    
    // 模糊匹配：仅比较文件名
    std::string targetFileName = pImpl_->pathNormalizer.getFileName(result.normalizedTargetPath);
    std::string declFileName = pImpl_->pathNormalizer.getFileName(result.normalizedDeclPath);
    
    result.isOwned = (targetFileName == declFileName);
    result.confidence = result.isOwned ? 0.3 : 0.0; // 低置信度
    result.reason = result.isOwned ? "Filename match (fuzzy)" : "Filename mismatch";
    
    return result;
}

bool FileOwnershipValidator::matchesExcludePattern(const std::string& filePath) const {
    for (const auto& pattern : pImpl_->excludePatterns) {
        try {
            std::regex regex(pattern);
            if (std::regex_search(filePath, regex)) {
                return true;
            }
        } catch (const std::exception& e) {
            // 如果正则表达式无效，尝试简单的通配符匹配
            if (filePath.find(pattern) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

double FileOwnershipValidator::calculatePathSimilarity(const std::string& path1, const std::string& path2) const {
    if (path1.empty() || path2.empty()) {
        return 0.0;
    }
    
    // 使用编辑距离计算相似度
    size_t len1 = path1.length();
    size_t len2 = path2.length();
    
    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));
    
    for (size_t i = 0; i <= len1; ++i) {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j) {
        dp[0][j] = j;
    }
    
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (path1[i-1] == path2[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
            }
        }
    }
    
    size_t editDistance = dp[len1][len2];
    size_t maxLen = std::max(len1, len2);
    
    return maxLen > 0 ? (1.0 - static_cast<double>(editDistance) / maxLen) : 0.0;
}

bool FileOwnershipValidator::isCorrespondingSourceFile(const std::string& headerFile, const std::string& sourceFile) const {
    std::string headerName = pImpl_->pathNormalizer.getFileName(headerFile);
    std::string sourceName = pImpl_->pathNormalizer.getFileName(sourceFile);
    
    // 移除扩展名
    size_t headerDot = headerName.find_last_of('.');
    size_t sourceDot = sourceName.find_last_of('.');
    
    if (headerDot == std::string::npos || sourceDot == std::string::npos) {
        return false;
    }
    
    std::string headerBase = headerName.substr(0, headerDot);
    std::string sourceBase = sourceName.substr(0, sourceDot);
    std::string headerExt = headerName.substr(headerDot);
    std::string sourceExt = sourceName.substr(sourceDot);
    
    // 检查基础名称是否相同
    if (headerBase != sourceBase) {
        return false;
    }
    
    // 检查扩展名组合
    std::vector<std::string> headerExts = {".h", ".hpp", ".hxx", ".h++", ".hh"};
    std::vector<std::string> sourceExts = {".cpp", ".cxx", ".c++", ".cc", ".c"};
    
    bool isHeaderExt = std::find(headerExts.begin(), headerExts.end(), headerExt) != headerExts.end();
    bool isSourceExt = std::find(sourceExts.begin(), sourceExts.end(), sourceExt) != sourceExts.end();
    
    return isHeaderExt && isSourceExt;
}

std::string FileOwnershipValidator::getCacheKey(const std::string& targetFile, const std::string& declFile, ValidationLevel level) const {
    return targetFile + "|" + declFile + "|" + std::to_string(static_cast<int>(level));
}

std::string validationLevelToString(FileOwnershipValidator::ValidationLevel level) {
    switch (level) {
        case FileOwnershipValidator::ValidationLevel::STRICT: return "STRICT";
        case FileOwnershipValidator::ValidationLevel::CANONICAL: return "CANONICAL";
        case FileOwnershipValidator::ValidationLevel::SMART: return "SMART";
        case FileOwnershipValidator::ValidationLevel::FUZZY: return "FUZZY";
        default: return "UNKNOWN";
    }
}

FileOwnershipValidator::ValidationLevel stringToValidationLevel(const std::string& str) {
    if (str == "STRICT") return FileOwnershipValidator::ValidationLevel::STRICT;
    if (str == "CANONICAL") return FileOwnershipValidator::ValidationLevel::CANONICAL;
    if (str == "SMART") return FileOwnershipValidator::ValidationLevel::SMART;
    if (str == "FUZZY") return FileOwnershipValidator::ValidationLevel::FUZZY;
    return FileOwnershipValidator::ValidationLevel::SMART; // 默认值
}

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover
 