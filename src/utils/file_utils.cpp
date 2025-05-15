/**
 * @file file_utils.cpp
 * @brief 文件工具函数实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/file_utils.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <random>

namespace fs = std::filesystem;

namespace dlogcover {
namespace utils {

bool FileUtils::fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool FileUtils::directoryExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

bool FileUtils::createDirectory(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (const std::exception&) {
        return false;
    }
}

bool FileUtils::readFile(const std::string& path, std::string& content) {
    try {
        if (!fileExists(path)) {
            return false;
        }

        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();

        file.close();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// 添加readFileToString作为readFile的别名
bool FileUtils::readFileToString(const std::string& path, std::string& content) {
    return readFile(path, content);
}

bool FileUtils::writeFile(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file << content;
        file.close();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

size_t FileUtils::getFileSize(const std::string& path) {
    try {
        if (!fileExists(path)) {
            return 0;
        }

        return static_cast<size_t>(fs::file_size(path));
    } catch (const std::exception&) {
        return 0;
    }
}

std::string FileUtils::getFileExtension(const std::string& path) {
    return fs::path(path).extension().string();
}

std::string FileUtils::getFileName(const std::string& path) {
    return fs::path(path).stem().string();
}

std::string FileUtils::getDirectoryName(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::vector<std::string> FileUtils::listFiles(const std::string& path,
                                  const std::string& pattern,
                                  bool recursive) {
    std::vector<std::string> files;

    try {
        if (!directoryExists(path)) {
            return files;
        }

        std::regex patternRegex;
        bool hasPattern = !pattern.empty();
        if (hasPattern) {
            try {
                patternRegex = std::regex(pattern, std::regex::ECMAScript);
            } catch (const std::regex_error&) {
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
    } catch (const std::exception&) {
        // 忽略异常
    }

    return files;
}

// 修改函数签名，返回文件列表而不是布尔值
std::vector<std::string> FileUtils::listFiles(const std::string& path,
              const std::function<bool(const std::string&)>& filter,
              const std::string& pattern,
              bool recursive) {
    std::vector<std::string> result;
    try {
        if (!directoryExists(path)) {
            return result;
        }

        std::regex patternRegex;
        bool hasPattern = !pattern.empty();
        if (hasPattern) {
            try {
                patternRegex = std::regex(pattern, std::regex::ECMAScript);
            } catch (const std::regex_error&) {
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

        return result;
    } catch (const std::exception&) {
        return result;
    }
}

std::string FileUtils::joinPaths(const std::string& path1, const std::string& path2) {
    return (fs::path(path1) / fs::path(path2)).string();
}

bool FileUtils::hasExtension(const std::string& filePath, const std::string& extension) {
    return fs::path(filePath).extension() == extension;
}

std::string FileUtils::createTempFile(const std::string& content) {
    try {
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
            return tempFilePath;
        }
    } catch (const std::exception&) {
        // 忽略异常
    }

    return "";
}

std::string FileUtils::normalizePath(const std::string& path) {
    return fs::path(path).lexically_normal().string();
}

std::string FileUtils::getRelativePath(const std::string& path, const std::string& basePath) {
    try {
        return fs::relative(path, basePath).string();
    } catch (const std::exception&) {
        return path;
    }
}

} // namespace utils
} // namespace dlogcover
