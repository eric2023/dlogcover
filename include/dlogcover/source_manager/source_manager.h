/**
 * @file source_manager.h
 * @brief 源文件管理器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_SOURCE_MANAGER_SOURCE_MANAGER_H
#define DLOGCOVER_SOURCE_MANAGER_SOURCE_MANAGER_H

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace dlogcover {
namespace source_manager {

/**
 * @brief 源文件信息结构
 */
struct SourceFileInfo {
    std::string path;           ///< 文件绝对路径
    std::string relativePath;   ///< 相对路径
    std::string content;        ///< 文件内容
    size_t size;               ///< 文件大小
    bool isHeader;             ///< 是否为头文件
};

/**
 * @brief 源文件管理器类
 *
 * 负责收集、管理和提供源文件信息
 */
class SourceManager {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     */
    explicit SourceManager(const config::Config& config);

    /**
     * @brief 析构函数
     */
    ~SourceManager();

    /**
     * @brief 收集源文件
     * @return 收集结果，成功返回true，否则返回错误信息
     */
    core::ast_analyzer::Result<bool> collectSourceFiles();

    /**
     * @brief 获取所有源文件信息
     * @return 源文件信息列表的常量引用
     */
    const std::vector<SourceFileInfo>& getSourceFiles() const;

    /**
     * @brief 获取源文件数量
     * @return 源文件数量
     */
    size_t getSourceFileCount() const;

    /**
     * @brief 根据路径获取源文件信息
     * @param path 文件路径
     * @return 源文件信息指针，未找到返回nullptr
     */
    const SourceFileInfo* getSourceFile(const std::string& path) const;

private:
    const config::Config& config_;                          ///< 配置对象引用
    std::vector<SourceFileInfo> sourceFiles_;               ///< 源文件信息列表
    std::unordered_map<std::string, size_t> pathToIndex_;   ///< 路径到索引的映射

    /**
     * @brief 检查文件是否应该被排除
     * @param path 文件路径
     * @return 如果应该排除返回true，否则返回false
     */
    bool shouldExclude(const std::string& path) const;

    /**
     * @brief 检查文件是否匹配include_patterns
     * @param path 文件路径
     * @return 如果应包含返回true，否则返回false
     */
    bool shouldInclude(const std::string& path) const;

    /**
     * @brief 检查文件类型是否被支持
     * @param path 文件路径
     * @return 如果支持返回true，否则返回false
     */
    bool isSupportedFileType(const std::string& path) const;

    /**
     * @brief 读取文件内容
     * @param path 文件路径
     * @param content 输出的文件内容
     * @return 读取成功返回true，否则返回false
     */
    bool readFileContent(const std::string& path, std::string& content) const;

    /**
     * @brief 将glob模式转换为正则表达式
     * @param glob glob模式字符串
     * @return 对应的正则表达式字符串
     */
    std::string globToRegex(const std::string& glob) const;
};

} // namespace source_manager
} // namespace dlogcover

#endif // DLOGCOVER_SOURCE_MANAGER_SOURCE_MANAGER_H
