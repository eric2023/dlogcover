/**
 * @file file_ownership_validator.h
 * @brief 文件归属验证引擎
 * @author DLogCover Team
 * @date 2024
 * 
 * 提供多层次的文件归属判断机制，确保AST分析中函数实现归属判断的准确性
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_FILE_OWNERSHIP_VALIDATOR_H
#define DLOGCOVER_CORE_AST_ANALYZER_FILE_OWNERSHIP_VALIDATOR_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_set>
#include "dlogcover/utils/path_normalizer.h"

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief 文件归属验证引擎
 * 
 * 提供多层次的文件归属判断机制，解决AST分析中函数实现归属判断的问题
 */
class FileOwnershipValidator {
public:
    /**
     * @brief 验证级别枚举
     */
    enum class ValidationLevel {
        STRICT,      ///< 严格模式：完全路径匹配
        CANONICAL,   ///< 规范化模式：规范化路径匹配
        SMART,       ///< 智能模式：考虑符号链接、相对路径等
        FUZZY        ///< 模糊模式：仅用于调试，不推荐生产使用
    };

    /**
     * @brief 验证结果结构
     */
    struct ValidationResult {
        bool isOwned;                    ///< 是否属于目标文件
        ValidationLevel usedLevel;       ///< 使用的验证级别
        std::string reason;              ///< 判断原因
        std::string normalizedDeclPath;  ///< 规范化的声明文件路径
        std::string normalizedTargetPath; ///< 规范化的目标文件路径
        double confidence;               ///< 置信度 (0.0-1.0)
        
        ValidationResult() 
            : isOwned(false), usedLevel(ValidationLevel::STRICT), confidence(0.0) {}
    };

    /**
     * @brief 构造函数
     */
    FileOwnershipValidator();

    /**
     * @brief 析构函数
     */
    ~FileOwnershipValidator();

    /**
     * @brief 验证文件归属
     * @param targetFile 目标文件路径
     * @param declFile 声明文件路径
     * @param level 验证级别
     * @return 验证结果
     */
    ValidationResult validateOwnership(
        const std::string& targetFile,
        const std::string& declFile,
        ValidationLevel level = ValidationLevel::SMART
    );

    /**
     * @brief 批量验证文件归属
     * @param targetFile 目标文件路径
     * @param declFiles 声明文件路径列表
     * @param level 验证级别
     * @return 验证结果列表
     */
    std::vector<ValidationResult> validateOwnershipBatch(
        const std::string& targetFile,
        const std::vector<std::string>& declFiles,
        ValidationLevel level = ValidationLevel::SMART
    );

    /**
     * @brief 设置项目根目录
     * @param projectRoot 项目根目录路径
     */
    void setProjectRoot(const std::string& projectRoot);

    /**
     * @brief 添加包含目录
     * @param includeDir 包含目录路径
     */
    void addIncludeDirectory(const std::string& includeDir);

    /**
     * @brief 添加排除模式
     * @param pattern 排除模式（支持通配符）
     */
    void addExcludePattern(const std::string& pattern);

    /**
     * @brief 清理缓存
     */
    void clearCache();

    /**
     * @brief 启用/禁用缓存
     * @param enabled 是否启用缓存
     */
    void setCacheEnabled(bool enabled);

    /**
     * @brief 设置调试模式
     * @param enabled 是否启用调试模式
     */
    void setDebugMode(bool enabled);

    /**
     * @brief 获取统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const;

private:
    /**
     * @brief 严格模式验证
     * @param targetFile 目标文件路径
     * @param declFile 声明文件路径
     * @return 验证结果
     */
    ValidationResult validateStrict(const std::string& targetFile, const std::string& declFile);

    /**
     * @brief 规范化模式验证
     * @param targetFile 目标文件路径
     * @param declFile 声明文件路径
     * @return 验证结果
     */
    ValidationResult validateCanonical(const std::string& targetFile, const std::string& declFile);

    /**
     * @brief 智能模式验证
     * @param targetFile 目标文件路径
     * @param declFile 声明文件路径
     * @return 验证结果
     */
    ValidationResult validateSmart(const std::string& targetFile, const std::string& declFile);

    /**
     * @brief 模糊模式验证
     * @param targetFile 目标文件路径
     * @param declFile 声明文件路径
     * @return 验证结果
     */
    ValidationResult validateFuzzy(const std::string& targetFile, const std::string& declFile);

    /**
     * @brief 检查是否匹配排除模式
     * @param filePath 文件路径
     * @return 是否匹配排除模式
     */
    bool matchesExcludePattern(const std::string& filePath) const;

    /**
     * @brief 计算路径相似度
     * @param path1 路径1
     * @param path2 路径2
     * @return 相似度 (0.0-1.0)
     */
    double calculatePathSimilarity(const std::string& path1, const std::string& path2) const;

    /**
     * @brief 检查是否为头文件对应的源文件
     * @param headerFile 头文件路径
     * @param sourceFile 源文件路径
     * @return 是否对应
     */
    bool isCorrespondingSourceFile(const std::string& headerFile, const std::string& sourceFile) const;

    /**
     * @brief 获取缓存键
     * @param targetFile 目标文件
     * @param declFile 声明文件
     * @param level 验证级别
     * @return 缓存键
     */
    std::string getCacheKey(const std::string& targetFile, const std::string& declFile, ValidationLevel level) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * @brief 验证级别转字符串
 * @param level 验证级别
 * @return 字符串表示
 */
std::string validationLevelToString(FileOwnershipValidator::ValidationLevel level);

/**
 * @brief 字符串转验证级别
 * @param str 字符串
 * @return 验证级别
 */
FileOwnershipValidator::ValidationLevel stringToValidationLevel(const std::string& str);

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_FILE_OWNERSHIP_VALIDATOR_H 