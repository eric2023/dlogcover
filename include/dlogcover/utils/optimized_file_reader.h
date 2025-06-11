/**
 * @file optimized_file_reader.h
 * @brief 优化的文件读取器 - 提供高效的文件I/O操作
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_OPTIMIZED_FILE_READER_H
#define DLOGCOVER_UTILS_OPTIMIZED_FILE_READER_H

#include <string>
#include <chrono>
#include <memory>
#include <vector>
#include <future>
#include <unordered_map>
#include <mutex>

namespace dlogcover {
namespace utils {

/**
 * @brief 文件读取结果
 */
struct FileReadResult {
    std::string content;                                     ///< 文件内容
    size_t fileSize;                                         ///< 文件大小
    std::chrono::system_clock::time_point lastModified;     ///< 最后修改时间
    bool success;                                            ///< 是否成功
    std::string errorMessage;                                ///< 错误消息
    std::chrono::milliseconds readTime;                     ///< 读取耗时

    /**
     * @brief 默认构造函数
     */
    FileReadResult() : fileSize(0), success(false), readTime(0) {}

    /**
     * @brief 成功结果构造函数
     */
    FileReadResult(std::string&& content, size_t size, 
                   const std::chrono::system_clock::time_point& modTime,
                   std::chrono::milliseconds time)
        : content(std::move(content)), fileSize(size), lastModified(modTime), 
          success(true), readTime(time) {}

    /**
     * @brief 失败结果构造函数
     */
    FileReadResult(const std::string& error)
        : fileSize(0), success(false), errorMessage(error), readTime(0) {}
};

/**
 * @brief 优化的文件读取器类
 * 
 * 提供多种优化的文件读取策略，根据文件大小自动选择最佳方法
 */
class OptimizedFileReader {
public:
    /**
     * @brief 读取文件
     * @param filePath 文件路径
     * @return 读取结果
     */
    static FileReadResult readFile(const std::string& filePath);

    /**
     * @brief 使用内存映射读取文件
     * @param filePath 文件路径
     * @return 读取结果
     */
    static FileReadResult readFileWithMmap(const std::string& filePath);

    /**
     * @brief 使用缓冲读取文件
     * @param filePath 文件路径
     * @return 读取结果
     */
    static FileReadResult readFileBuffered(const std::string& filePath);

    /**
     * @brief 批量读取文件
     * @param filePaths 文件路径列表
     * @return 读取结果映射表
     */
    static std::unordered_map<std::string, FileReadResult> readFilesParallel(
        const std::vector<std::string>& filePaths);

    /**
     * @brief 设置缓冲区大小
     * @param size 缓冲区大小（字节）
     */
    static void setBufferSize(size_t size);

    /**
     * @brief 设置内存映射阈值
     * @param threshold 阈值（字节）
     */
    static void setMmapThreshold(size_t threshold);

    /**
     * @brief 获取读取统计信息
     * @return 统计信息字符串
     */
    static std::string getStatistics();

private:
    static constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1024;     ///< 默认缓冲区大小 64KB
    static constexpr size_t DEFAULT_MMAP_THRESHOLD = 1024 * 1024; ///< 默认内存映射阈值 1MB

    static size_t bufferSize_;                                   ///< 缓冲区大小
    static size_t mmapThreshold_;                               ///< 内存映射阈值
    static std::atomic<size_t> totalFilesRead_;                 ///< 总读取文件数
    static std::atomic<size_t> totalBytesRead_;                 ///< 总读取字节数
    static std::atomic<size_t> totalReadTime_;                  ///< 总读取时间（毫秒）
    static std::atomic<size_t> mmapReads_;                      ///< 内存映射读取次数
    static std::atomic<size_t> bufferedReads_;                  ///< 缓冲读取次数

    /**
     * @brief 获取文件信息
     * @param filePath 文件路径
     * @param size 输出文件大小
     * @param lastModified 输出最后修改时间
     * @return 是否成功
     */
    static bool getFileInfo(const std::string& filePath, size_t& size, 
                           std::chrono::system_clock::time_point& lastModified);

    /**
     * @brief 更新统计信息
     * @param bytesRead 读取字节数
     * @param readTime 读取时间
     * @param usedMmap 是否使用内存映射
     */
    static void updateStatistics(size_t bytesRead, std::chrono::milliseconds readTime, bool usedMmap);
};

/**
 * @brief 文件预加载器类
 * 
 * 异步预加载文件，减少I/O等待时间
 */
class FilePreloader {
public:
    /**
     * @brief 构造函数
     */
    FilePreloader();

    /**
     * @brief 析构函数
     */
    ~FilePreloader();

    /**
     * @brief 预加载文件
     * @param filePath 文件路径
     */
    void preloadFile(const std::string& filePath);

    /**
     * @brief 获取预加载的文件
     * @param filePath 文件路径
     * @return 文件读取结果
     */
    FileReadResult getPreloadedFile(const std::string& filePath);

    /**
     * @brief 批量预加载文件
     * @param filePaths 文件路径列表
     */
    void preloadFiles(const std::vector<std::string>& filePaths);

    /**
     * @brief 清空预加载缓存
     */
    void clearCache();

    /**
     * @brief 关闭预加载器
     */
    void shutdown();

    /**
     * @brief 获取预加载统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const;

private:
    std::unordered_map<std::string, std::future<FileReadResult>> preloadedFiles_; ///< 预加载文件映射
    std::mutex preloadMutex_;                                                     ///< 预加载互斥锁
    std::atomic<bool> shouldStop_{false};                                        ///< 是否应该停止
    std::atomic<size_t> preloadHits_{0};                                         ///< 预加载命中次数
    std::atomic<size_t> preloadMisses_{0};                                       ///< 预加载未命中次数

    /**
     * @brief 清理已完成的预加载任务
     */
    void cleanupCompletedTasks();
};

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_OPTIMIZED_FILE_READER_H 