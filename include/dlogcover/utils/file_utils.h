/**
 * @file file_utils.h
 * @brief 文件工具函数
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_FILE_UTILS_H
#define DLOGCOVER_UTILS_FILE_UTILS_H

#include <string>
#include <vector>

namespace dlogcover {
namespace utils {

/**
 * @brief 临时文件类型枚举
 */
enum class TempFileType {
    FILE = 0,       ///< 普通文件
    DIRECTORY = 1   ///< 目录
};

/**
 * @brief 文件工具类
 * 
 * 提供文件和目录操作的工具函数
 */
class FileUtils {
public:
    /**
     * @brief 检查文件是否存在
     * @param path 文件路径
     * @return 如果文件存在返回true，否则返回false
     */
    static bool fileExists(const std::string& path);

    /**
     * @brief 检查目录是否存在
     * @param path 目录路径
     * @return 如果目录存在返回true，否则返回false
     */
    static bool directoryExists(const std::string& path);

    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return 创建成功返回true，否则返回false
     */
    static bool createDirectory(const std::string& path);

    /**
     * @brief 读取文件内容
     * @param path 文件路径
     * @param content 输出的文件内容
     * @return 读取成功返回true，否则返回false
     */
    static bool readFile(const std::string& path, std::string& content);

    /**
     * @brief 读取文件内容（废弃的方法）
     * @param path 文件路径
     * @param content 输出的文件内容
     * @return 读取成功返回true，否则返回false
     * @deprecated 请使用 readFile 代替
     */
    [[deprecated("请使用 readFile 代替")]]
    static bool readFileToString(const std::string& path, std::string& content);

    /**
     * @brief 写入文件内容
     * @param path 文件路径
     * @param content 要写入的内容
     * @return 写入成功返回true，否则返回false
     */
    static bool writeFile(const std::string& path, const std::string& content);

    /**
     * @brief 获取文件大小
     * @param path 文件路径
     * @return 文件大小（字节）
     */
    static size_t getFileSize(const std::string& path);

    /**
     * @brief 获取文件扩展名
     * @param path 文件路径
     * @return 文件扩展名（包含点号）
     */
    static std::string getFileExtension(const std::string& path);

    /**
     * @brief 获取文件名（不含扩展名）
     * @param path 文件路径
     * @return 文件名
     */
    static std::string getFileName(const std::string& path);

    /**
     * @brief 获取目录名
     * @param path 文件路径
     * @return 目录路径
     */
    static std::string getDirectoryName(const std::string& path);

    /**
     * @brief 列出目录中的文件
     * @param path 目录路径
     * @param pattern 文件名模式（正则表达式）
     * @param recursive 是否递归查找
     * @return 文件路径列表
     */
    static std::vector<std::string> listFiles(const std::string& path, 
                                            const std::string& pattern = "", 
                                            bool recursive = false);

    /**
     * @brief 列出目录中的子目录
     * @param path 目录路径
     * @param pattern 目录名模式（正则表达式）
     * @param recursive 是否递归查找
     * @return 目录路径列表
     */
    static std::vector<std::string> listDirectories(const std::string& path,
                                                   const std::string& pattern = "",
                                                   bool recursive = false);

    /**
     * @brief 创建临时目录
     * @param prefix 目录名前缀
     * @return 临时目录路径
     */
    static std::string createTempDirectory(const std::string& prefix = "dlogcover_");

    /**
     * @brief 创建临时文件
     * @param prefix 文件名前缀
     * @param type 临时文件类型
     * @return 临时文件路径
     */
    static std::string createTempFile(const std::string& prefix = "dlogcover_", 
                                    TempFileType type = TempFileType::FILE);

    /**
     * @brief 清理临时文件
     */
    static void cleanupTempFiles();

    /**
     * @brief 判断路径是否为绝对路径
     * @param path 路径
     * @return 如果是绝对路径返回true，否则返回false
     */
    static bool isAbsolutePath(const std::string& path);

    /**
     * @brief 获取绝对路径
     * @param path 相对或绝对路径
     * @return 绝对路径
     */
    static std::string getAbsolutePath(const std::string& path);

    /**
     * @brief 获取相对路径
     * @param from 起始路径
     * @param to 目标路径
     * @return 相对路径
     */
    static std::string getRelativePath(const std::string& from, const std::string& to);

    /**
     * @brief 规范化路径
     * @param path 原始路径
     * @return 规范化后的路径
     */
    static std::string normalizePath(const std::string& path);

    /**
     * @brief 删除文件
     * @param path 文件路径
     * @return 删除成功返回true，否则返回false
     */
    static bool removeFile(const std::string& path);

    /**
     * @brief 删除目录（递归删除）
     * @param path 目录路径
     * @return 删除成功返回true，否则返回false
     */
    static bool removeDirectory(const std::string& path);

    /**
     * @brief 复制文件
     * @param from 源文件路径
     * @param to 目标文件路径
     * @return 复制成功返回true，否则返回false
     */
    static bool copyFile(const std::string& from, const std::string& to);

    /**
     * @brief 移动文件
     * @param from 源文件路径
     * @param to 目标文件路径
     * @return 移动成功返回true，否则返回false
     */
    static bool moveFile(const std::string& from, const std::string& to);
};

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_FILE_UTILS_H 