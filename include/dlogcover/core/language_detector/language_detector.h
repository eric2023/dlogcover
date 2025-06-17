/**
 * @file language_detector.h
 * @brief 语言检测器 - 基于文件扩展名检测源代码语言类型
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_LANGUAGE_DETECTOR_LANGUAGE_DETECTOR_H
#define DLOGCOVER_CORE_LANGUAGE_DETECTOR_LANGUAGE_DETECTOR_H

#include <string>
#include <vector>

namespace dlogcover {
namespace core {
namespace language_detector {

/**
 * @brief 支持的源代码语言类型
 */
enum class SourceLanguage {
    CPP,        ///< C++语言
    GO,         ///< Go语言
    UNKNOWN     ///< 未知语言类型
};

/**
 * @brief 语言检测器类
 * 
 * 负责根据文件扩展名检测源代码的语言类型
 */
class LanguageDetector {
public:
    /**
     * @brief 检测文件的语言类型
     * @param filePath 文件路径
     * @return 检测到的语言类型
     */
    static SourceLanguage detectLanguage(const std::string& filePath);

    /**
     * @brief 检查是否为C++文件扩展名
     * @param path 文件路径
     * @return 如果是C++文件返回true，否则返回false
     */
    static bool hasCppExtension(const std::string& path);

    /**
     * @brief 检查是否为Go文件扩展名
     * @param path 文件路径
     * @return 如果是Go文件返回true，否则返回false
     */
    static bool hasGoExtension(const std::string& path);

    /**
     * @brief 获取语言类型的字符串表示
     * @param language 语言类型
     * @return 语言类型的字符串名称
     */
    static std::string getLanguageName(SourceLanguage language);

private:
    /**
     * @brief 检查文件是否具有指定的任一扩展名
     * @param path 文件路径
     * @param extensions 扩展名列表
     * @return 如果匹配任一扩展名返回true，否则返回false
     */
    static bool hasAnyExtension(const std::string& path, const std::vector<std::string>& extensions);

    /**
     * @brief 获取文件扩展名（小写）
     * @param path 文件路径
     * @return 文件扩展名，如果没有扩展名返回空字符串
     */
    static std::string getFileExtension(const std::string& path);

    /// C++文件扩展名列表
    static const std::vector<std::string> CPP_EXTENSIONS;
    
    /// Go文件扩展名列表
    static const std::vector<std::string> GO_EXTENSIONS;
};

} // namespace language_detector
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_LANGUAGE_DETECTOR_LANGUAGE_DETECTOR_H 