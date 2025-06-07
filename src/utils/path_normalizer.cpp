/**
 * @file path_normalizer.cpp
 * @brief 路径规范化工具实现
 * @author DLogCover Team
 * @date 2024
 */

#include "dlogcover/utils/path_normalizer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace dlogcover {
namespace utils {

std::string PathNormalizer::normalize(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    try {
        std::filesystem::path fsPath(path);
        std::filesystem::path normalized = fsPath.lexically_normal();
        return normalized.string();
    } catch (const std::exception& e) {
        // 如果filesystem操作失败，使用手动清理
        return cleanPath(path);
    }
}

std::string PathNormalizer::getCanonicalPath(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    try {
        std::filesystem::path fsPath(path);
        
        // 如果路径不存在，先规范化再转换为绝对路径
        if (!std::filesystem::exists(fsPath)) {
            fsPath = fsPath.lexically_normal();
            if (fsPath.is_relative()) {
                fsPath = std::filesystem::current_path() / fsPath;
            }
            return fsPath.lexically_normal().string();
        }
        
        // 路径存在时，获取真实的规范路径
        std::filesystem::path canonical = std::filesystem::canonical(fsPath);
        return canonical.string();
    } catch (const std::exception& e) {
        // 降级处理：手动构建绝对路径
        std::string normalized = normalize(path);
        if (isAbsolutePath(normalized)) {
            return normalized;
        }
        
        try {
            std::filesystem::path currentDir = std::filesystem::current_path();
            std::filesystem::path absolutePath = currentDir / normalized;
            return absolutePath.lexically_normal().string();
        } catch (const std::exception& e2) {
            return normalized;
        }
    }
}

bool PathNormalizer::isSamePath(const std::string& path1, const std::string& path2) {
    if (path1.empty() || path2.empty()) {
        return path1 == path2;
    }

    // 首先尝试规范化比较
    std::string canonical1 = getCanonicalPath(path1);
    std::string canonical2 = getCanonicalPath(path2);
    
    if (canonical1 == canonical2) {
        return true;
    }

    // 如果规范化路径不同，尝试解析符号链接后比较
    try {
        std::string resolved1 = resolveSymlinks(canonical1);
        std::string resolved2 = resolveSymlinks(canonical2);
        return resolved1 == resolved2;
    } catch (const std::exception& e) {
        // 如果解析失败，回退到字符串比较
        return canonical1 == canonical2;
    }
}

std::string PathNormalizer::getRelativePath(const std::string& from, const std::string& to) {
    if (from.empty() || to.empty()) {
        return to;
    }

    try {
        std::filesystem::path fromPath = std::filesystem::path(from).lexically_normal();
        std::filesystem::path toPath = std::filesystem::path(to).lexically_normal();
        
        // 确保都是绝对路径
        if (fromPath.is_relative()) {
            fromPath = std::filesystem::current_path() / fromPath;
            fromPath = fromPath.lexically_normal();
        }
        if (toPath.is_relative()) {
            toPath = std::filesystem::current_path() / toPath;
            toPath = toPath.lexically_normal();
        }
        
        std::filesystem::path relativePath = std::filesystem::relative(toPath, fromPath);
        return relativePath.string();
    } catch (const std::exception& e) {
        // 降级处理：返回目标路径
        return normalize(to);
    }
}

bool PathNormalizer::isAbsolutePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        std::filesystem::path fsPath(path);
        return fsPath.is_absolute();
    } catch (const std::exception& e) {
        // 手动检查
        return (path[0] == '/' || (path.length() >= 3 && path[1] == ':' && path[2] == '\\'));
    }
}

std::string PathNormalizer::getFileName(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    try {
        std::filesystem::path fsPath(path);
        return fsPath.filename().string();
    } catch (const std::exception& e) {
        // 手动提取文件名
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            return path.substr(lastSlash + 1);
        }
        return path;
    }
}

std::string PathNormalizer::getDirectoryPath(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    try {
        std::filesystem::path fsPath(path);
        return fsPath.parent_path().string();
    } catch (const std::exception& e) {
        // 手动提取目录路径
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            return path.substr(0, lastSlash);
        }
        return "";
    }
}

bool PathNormalizer::exists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return std::filesystem::exists(path);
    } catch (const std::exception& e) {
        return false;
    }
}

std::string PathNormalizer::cleanPath(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string component;
    
    // 分割路径组件
    char delimiter = (path.find('/') != std::string::npos) ? '/' : '\\';
    while (std::getline(ss, component, delimiter)) {
        if (component.empty() || component == ".") {
            continue;
        }
        if (component == "..") {
            if (!components.empty() && components.back() != "..") {
                components.pop_back();
            } else if (path[0] != '/' && path[0] != '\\') {
                // 相对路径可以保留 ..
                components.push_back(component);
            }
        } else {
            components.push_back(component);
        }
    }

    // 重建路径
    std::string result;
    if (path[0] == '/' || path[0] == '\\') {
        result += delimiter;
    }
    
    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += components[i];
    }

    return result.empty() ? "." : result;
}

std::string PathNormalizer::resolveSymlinks(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::path resolved = std::filesystem::canonical(path);
            return resolved.string();
        }
    } catch (const std::exception& e) {
        // 如果无法解析符号链接，返回原路径
    }
    
    return path;
}

} // namespace utils
} // namespace dlogcover 