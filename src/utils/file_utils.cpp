/**
 * @file file_utils.cpp
 * @brief 文件工具函数实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <random>
#include <system_error>

namespace fs = std::filesystem;

namespace dlogcover {
namespace utils {

// 用于临时文件管理的静态变量
namespace {
    std::vector<std::string> tempFilesToCleanup;
    std::mutex tempFilesMutex;
}

bool FileUtils::fileExists(const std::string& path) {
    try {
        bool exists = fs::exists(path) && fs::is_regular_file(path);
        LOG_DEBUG_FMT("检查文件是否存在: %s, 结果: %s", path.c_str(), exists ? "存在" : "不存在");
        return exists;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR_FMT("检查文件存在性失败: %s, 错误: %s", path.c_str(), e.what());
        return false;
    }
}

bool FileUtils::directoryExists(const std::string& path) {
    try {
        bool exists = fs::exists(path) && fs::is_directory(path);
        LOG_DEBUG_FMT("检查目录是否存在: %s, 结果: %s", path.c_str(), exists ? "存在" : "不存在");
        return exists;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR_FMT("检查目录存在性失败: %s, 错误: %s", path.c_str(), e.what());
        return false;
    }
}

bool FileUtils::createDirectory(const std::string& path) {
    try {
        LOG_DEBUG_FMT("创建目录: %s", path.c_str());
        bool result = fs::create_directories(path);
        if (result) {
            LOG_INFO_FMT("已成功创建目录: %s", path.c_str());
        } else {
            LOG_WARNING_FMT("目录可能已存在: %s", path.c_str());
        }
        return result;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR_FMT("创建目录失败: %s, 错误: %s", path.c_str(), e.what());
        return false;
    }
}

bool FileUtils::readFile(const std::string& path, std::string& content) {
    try {
        LOG_DEBUG_FMT("读取文件: %s", path.c_str());

        if (!fileExists(path)) {
            LOG_ERROR_FMT("文件不存在: %s", path.c_str());
            return false;
        }

        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR_FMT("无法打开文件: %s", path.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();

        file.close();

        LOG_DEBUG_FMT("已读取文件: %s, 大小: %zu 字节", path.c_str(), content.size());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("读取文件失败: %s, 错误: %s", path.c_str(), e.what());
        return false;
    }
}

// 保留 readFileToString 作为 readFile 的别名，但标记为废弃
[[deprecated("请使用 readFile 代替")]]
bool FileUtils::readFileToString(const std::string& path, std::string& content) {
    return readFile(path, content);
}

bool FileUtils::writeFile(const std::string& path, const std::string& content) {
    try {
        LOG_DEBUG_FMT("写入文件: %s, 大小: %zu 字节", path.c_str(), content.size());

        // 确保目录存在
        std::string dirPath = getDirectoryName(path);
        if (!dirPath.empty() && !directoryExists(dirPath)) {
            LOG_INFO_FMT("创建文件的父目录: %s", dirPath.c_str());
            createDirectory(dirPath);
        }

        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR_FMT("无法打开文件进行写入: %s", path.c_str());
            return false;
        }

        file << content;
        file.close();

        LOG_DEBUG_FMT("已写入文件: %s", path.c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("写入文件失败: %s, 错误: %s", path.c_str(), e.what());
        return false;
    }
}

size_t FileUtils::getFileSize(const std::string& path) {
    try {
        if (!fileExists(path)) {
            LOG_WARNING_FMT("无法获取不存在文件的大小: %s", path.c_str());
            return 0;
        }

        size_t size = static_cast<size_t>(fs::file_size(path));
        LOG_DEBUG_FMT("文件大小: %s, %zu 字节", path.c_str(), size);
        return size;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR_FMT("获取文件大小失败: %s, 错误: %s", path.c_str(), e.what());
        return 0;
    }
}

std::string FileUtils::getFileExtension(const std::string& path) {
    try {
        std::string ext = fs::path(path).extension().string();
        LOG_DEBUG_FMT("文件扩展名: %s, %s", path.c_str(), ext.c_str());
        return ext;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("获取文件扩展名失败: %s, 错误: %s", path.c_str(), e.what());
        return "";
    }
}

std::string FileUtils::getFileName(const std::string& path) {
    try {
        std::string name = fs::path(path).stem().string();
        LOG_DEBUG_FMT("文件名: %s, %s", path.c_str(), name.c_str());
        return name;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("获取文件名失败: %s, 错误: %s", path.c_str(), e.what());
        return "";
    }
}

std::string FileUtils::getDirectoryName(const std::string& path) {
    try {
        std::string dir = fs::path(path).parent_path().string();
        LOG_DEBUG_FMT("目录名: %s, %s", path.c_str(), dir.c_str());
        return dir;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("获取目录名失败: %s, 错误: %s", path.c_str(), e.what());
        return "";
    }
}

std::vector<std::string> FileUtils::listFiles(const std::string& path,
                                              const std::string& pattern,
                                              bool recursive) {
    std::vector<std::string> files;

    try {
        LOG_DEBUG_FMT("列出文件: 目录: %s, 模式: %s, 递归: %s",
                     path.c_str(),
                     pattern.empty() ? "<无>" : pattern.c_str(),
                     recursive ? "是" : "否");

        if (!directoryExists(path)) {
            LOG_WARNING_FMT("目录不存在: %s", path.c_str());
            return files;
        }

        std::regex patternRegex;
        bool hasPattern = !pattern.empty();
        if (hasPattern) {
            try {
                patternRegex = std::regex(pattern, std::regex::ECMAScript);
            } catch (const std::regex_error& e) {
                LOG_ERROR_FMT("无效的正则表达式模式: %s, 错误: %s", pattern.c_str(), e.what());
                hasPattern = false;
            }
        }

        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    if (!hasPattern || std::regex_match(filePath, patternRegex)) {
                        files.push_back(filePath);
                    }
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    if (!hasPattern || std::regex_match(filePath, patternRegex)) {
                        files.push_back(filePath);
                    }
                }
            }
        }

        LOG_DEBUG_FMT("已找到 %zu 个文件", files.size());
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR_FMT("列出文件失败: %s, 错误: %s", path.c_str(), e.what());
    }

    return files;
}

std::vector<std::string> FileUtils::listFiles(const std::string& path,
                                              const std::function<bool(const std::string&)>& filter,
                                              const std::string& pattern,
                                              bool recursive) {
    std::vector<std::string> result;
    try {
        LOG_DEBUG_FMT("列出文件(带过滤器): 目录: %s, 模式: %s, 递归: %s",
                     path.c_str(),
                     pattern.empty() ? "<无>" : pattern.c_str(),
                     recursive ? "是" : "否");

        if (!directoryExists(path)) {
            LOG_WARNING_FMT("目录不存在: %s", path.c_str());
            return result;
        }

        std::regex patternRegex;
        bool hasPattern = !pattern.empty();
        if (hasPattern) {
            try {
                patternRegex = std::regex(pattern, std::regex::ECMAScript);
            } catch (const std::regex_error& e) {
                LOG_ERROR_FMT("无效的正则表达式模式: %s, 错误: %s", pattern.c_str(), e.what());
                hasPattern = false;
            }
        }

        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    if ((!hasPattern || std::regex_match(filePath, patternRegex)) && filter(filePath)) {
                        result.push_back(filePath);
                    }
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    if ((!hasPattern || std::regex_match(filePath, patternRegex)) && filter(filePath)) {
                        result.push_back(filePath);
                    }
                }
            }
        }

        LOG_DEBUG_FMT("已找到 %zu 个符合过滤条件的文件", result.size());
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR_FMT("列出文件(带过滤器)失败: %s, 错误: %s", path.c_str(), e.what());
    }

    return result;
}

std::string FileUtils::joinPaths(const std::string& path1, const std::string& path2) {
    try {
        std::string result = (fs::path(path1) / fs::path(path2)).string();
        LOG_DEBUG_FMT("连接路径: %s + %s = %s", path1.c_str(), path2.c_str(), result.c_str());
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("连接路径失败: %s + %s, 错误: %s", path1.c_str(), path2.c_str(), e.what());
        return path1 + "/" + path2;  // 回退到简单的连接方式
    }
}

bool FileUtils::hasExtension(const std::string& filePath, const std::string& extension) {
    try {
        bool has = fs::path(filePath).extension() == extension;
        LOG_DEBUG_FMT("检查文件扩展名: %s, 目标扩展名: %s, 结果: %s",
                     filePath.c_str(), extension.c_str(), has ? "匹配" : "不匹配");
        return has;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("检查文件扩展名失败: %s, 错误: %s", filePath.c_str(), e.what());
        return false;
    }
}

std::string FileUtils::createTempFile(const std::string& content) {
    try {
        LOG_DEBUG_FMT("创建带内容的临时文件, 内容大小: %zu 字节", content.size());

        // 生成随机文件名
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 35); // 0-9, a-z

        std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string randomStr;
        for (int i = 0; i < 10; ++i) {
            randomStr += chars[dis(gen)];
        }

        // 创建临时文件路径
        fs::path tempDir = fs::temp_directory_path();
        fs::path tempFile = tempDir / ("dlogcover_temp_" + randomStr + ".tmp");
        std::string tempFilePath = tempFile.string();

        // 写入内容
        if (writeFile(tempFilePath, content)) {
            // 将临时文件添加到清理列表
            std::lock_guard<std::mutex> lock(tempFilesMutex);
            tempFilesToCleanup.push_back(tempFilePath);
            LOG_INFO_FMT("已创建临时文件: %s", tempFilePath.c_str());
            return tempFilePath;
        }
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("创建临时文件失败: %s", e.what());
    }

    return "";
}

std::string FileUtils::createTempFile(const std::string& prefix, TempFileType type) {
    try {
        LOG_DEBUG_FMT("创建临时文件, 前缀: %s, 类型: %d", prefix.c_str(), static_cast<int>(type));

        // 生成随机文件名
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 35); // 0-9, a-z

        std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string randomStr;
        for (int i = 0; i < 10; ++i) {
            randomStr += chars[dis(gen)];
        }

        // 创建临时文件路径
        fs::path tempDir = fs::temp_directory_path();
        fs::path tempFile = tempDir / (prefix + "_" + randomStr + ".tmp");
        std::string tempFilePath = tempFile.string();

        // 根据类型写入或创建文件
        if (type == TempFileType::EMPTY) {
            std::ofstream file(tempFilePath);
            if (file.is_open()) {
                file.close();
                // 将临时文件添加到清理列表
                std::lock_guard<std::mutex> lock(tempFilesMutex);
                tempFilesToCleanup.push_back(tempFilePath);
                LOG_INFO_FMT("已创建空临时文件: %s", tempFilePath.c_str());
                return tempFilePath;
            }
        } else if (type == TempFileType::WITH_CONTENT) {
            std::string content = "This is a temporary file created by DLogCover.";
            if (writeFile(tempFilePath, content)) {
                // 将临时文件添加到清理列表
                std::lock_guard<std::mutex> lock(tempFilesMutex);
                tempFilesToCleanup.push_back(tempFilePath);
                LOG_INFO_FMT("已创建带默认内容的临时文件: %s", tempFilePath.c_str());
                return tempFilePath;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("创建临时文件失败: %s", e.what());
    }

    return "";
}

void FileUtils::cleanupTempFiles() {
    std::lock_guard<std::mutex> lock(tempFilesMutex);

    LOG_DEBUG_FMT("开始清理临时文件, 共 %zu 个", tempFilesToCleanup.size());

    for (const auto& file : tempFilesToCleanup) {
        try {
            if (fs::exists(file) && fs::is_regular_file(file)) {
                fs::remove(file);
                LOG_DEBUG_FMT("已删除临时文件: %s", file.c_str());
            }
        } catch (const std::exception& e) {
            LOG_WARNING_FMT("删除临时文件失败: %s, 错误: %s", file.c_str(), e.what());
        }
    }

    tempFilesToCleanup.clear();
    LOG_INFO("所有临时文件已清理");
}

std::string FileUtils::normalizePath(const std::string& path) {
    try {
        std::string normalized = fs::path(path).lexically_normal().string();
        LOG_DEBUG_FMT("规范化路径: %s -> %s", path.c_str(), normalized.c_str());
        return normalized;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("规范化路径失败: %s, 错误: %s", path.c_str(), e.what());
        return path;
    }
}

std::string FileUtils::getRelativePath(const std::string& path, const std::string& basePath) {
    try {
        std::string relative = fs::relative(path, basePath).string();
        LOG_DEBUG_FMT("获取相对路径: %s 相对于 %s = %s", path.c_str(), basePath.c_str(), relative.c_str());
        return relative;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("获取相对路径失败: %s 相对于 %s, 错误: %s", path.c_str(), basePath.c_str(), e.what());
        return path;
    }
}

} // namespace utils
} // namespace dlogcover
