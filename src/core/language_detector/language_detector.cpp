/**
 * @file language_detector.cpp
 * @brief 语言检测器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/language_detector/language_detector.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <filesystem>

namespace dlogcover {
namespace core {
namespace language_detector {

// 静态成员变量定义
const std::vector<std::string> LanguageDetector::CPP_EXTENSIONS = {
    ".cpp", ".cxx", ".cc", ".c++", ".C",  // C++源文件
    ".h", ".hpp", ".hxx", ".h++", ".hh"   // C++头文件
};

const std::vector<std::string> LanguageDetector::GO_EXTENSIONS = {
    ".go"  // Go源文件
};

SourceLanguage LanguageDetector::detectLanguage(const std::string& filePath) {
    LOG_DEBUG_FMT("检测文件语言类型: %s", filePath.c_str());
    
    if (filePath.empty()) {
        LOG_WARNING("文件路径为空，无法检测语言类型");
        return SourceLanguage::UNKNOWN;
    }
    
    // 检查C++文件扩展名
    if (hasCppExtension(filePath)) {
        LOG_DEBUG_FMT("检测到C++文件: %s", filePath.c_str());
        return SourceLanguage::CPP;
    }
    
    // 检查Go文件扩展名
    if (hasGoExtension(filePath)) {
        LOG_DEBUG_FMT("检测到Go文件: %s", filePath.c_str());
        return SourceLanguage::GO;
    }
    
    LOG_DEBUG_FMT("未知文件类型: %s", filePath.c_str());
    return SourceLanguage::UNKNOWN;
}

bool LanguageDetector::hasCppExtension(const std::string& path) {
    return hasAnyExtension(path, CPP_EXTENSIONS);
}

bool LanguageDetector::hasGoExtension(const std::string& path) {
    return hasAnyExtension(path, GO_EXTENSIONS);
}

std::string LanguageDetector::getLanguageName(SourceLanguage language) {
    switch (language) {
        case SourceLanguage::CPP:
            return "C++";
        case SourceLanguage::GO:
            return "Go";
        case SourceLanguage::UNKNOWN:
        default:
            return "Unknown";
    }
}

bool LanguageDetector::hasAnyExtension(const std::string& path, const std::vector<std::string>& extensions) {
    if (path.empty() || extensions.empty()) {
        return false;
    }
    
    std::string fileExt = getFileExtension(path);
    if (fileExt.empty()) {
        return false;
    }
    
    // 在扩展名列表中查找匹配项
    return std::find(extensions.begin(), extensions.end(), fileExt) != extensions.end();
}

std::string LanguageDetector::getFileExtension(const std::string& path) {
    try {
        std::filesystem::path filePath(path);
        std::string extension = filePath.extension().string();
        
        // 转换为小写以进行不区分大小写的比较
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        return extension;
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("获取文件扩展名失败: %s, 错误: %s", path.c_str(), e.what());
        return "";
    }
}

} // namespace language_detector
} // namespace core
} // namespace dlogcover 