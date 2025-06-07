/**
 * @file path_normalizer.h
 * @brief 路径规范化工具
 * @author DLogCover Team
 * @date 2024
 * 
 * 提供统一的路径处理和规范化功能，确保路径比较的准确性
 */

#ifndef DLOGCOVER_UTILS_PATH_NORMALIZER_H
#define DLOGCOVER_UTILS_PATH_NORMALIZER_H

#include <string>
#include <filesystem>
#include <vector>

namespace dlogcover {
namespace utils {

/**
 * @brief 路径规范化工具类
 * 
 * 提供各种路径处理功能，包括规范化、比较、相对路径计算等
 */
class PathNormalizer {
public:
    /**
     * @brief 规范化路径
     * @param path 原始路径
     * @return 规范化后的路径
     * 
     * 处理以下情况：
     * - 统一路径分隔符
     * - 移除多余的 . 和 ..
     * - 处理符号链接
     */
    static std::string normalize(const std::string& path);

    /**
     * @brief 获取规范路径（绝对路径）
     * @param path 原始路径
     * @return 规范的绝对路径
     * 
     * 将相对路径转换为绝对路径，并进行规范化
     */
    static std::string getCanonicalPath(const std::string& path);

    /**
     * @brief 判断两个路径是否指向同一文件
     * @param path1 第一个路径
     * @param path2 第二个路径
     * @return 是否为同一文件
     * 
     * 考虑符号链接、相对路径等情况
     */
    static bool isSamePath(const std::string& path1, const std::string& path2);

    /**
     * @brief 计算相对路径
     * @param from 起始路径
     * @param to 目标路径
     * @return 从from到to的相对路径
     */
    static std::string getRelativePath(const std::string& from, const std::string& to);

    /**
     * @brief 检查路径是否为绝对路径
     * @param path 待检查的路径
     * @return 是否为绝对路径
     */
    static bool isAbsolutePath(const std::string& path);

    /**
     * @brief 获取文件名（不含路径）
     * @param path 文件路径
     * @return 文件名
     */
    static std::string getFileName(const std::string& path);

    /**
     * @brief 获取目录路径
     * @param path 文件路径
     * @return 目录路径
     */
    static std::string getDirectoryPath(const std::string& path);

    /**
     * @brief 检查路径是否存在
     * @param path 待检查的路径
     * @return 路径是否存在
     */
    static bool exists(const std::string& path);

private:
    /**
     * @brief 内部路径清理函数
     * @param path 原始路径
     * @return 清理后的路径
     */
    static std::string cleanPath(const std::string& path);

    /**
     * @brief 解析符号链接
     * @param path 可能包含符号链接的路径
     * @return 解析后的真实路径
     */
    static std::string resolveSymlinks(const std::string& path);
};

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_PATH_NORMALIZER_H 