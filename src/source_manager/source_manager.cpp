/**
 * @file source_manager.cpp
 * @brief 源文件管理器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>

namespace dlogcover {
namespace source_manager {

SourceManager::SourceManager(const config::Config& config) : config_(config) {
    LOG_DEBUG("源文件管理器初始化");
}

SourceManager::~SourceManager() {
    LOG_DEBUG("源文件管理器销毁");
}

core::ast_analyzer::Result<bool> SourceManager::collectSourceFiles() {
    LOG_INFO("开始收集源文件");

    try {
        sourceFiles_.clear();
        pathToIndex_.clear();

        // 优先处理include_patterns，支持glob匹配
        if (!config_.scan.include_patterns.empty()) {
            LOG_INFO_FMT("仅扫描include_patterns指定的文件/目录（支持glob），数量: %zu",
                         config_.scan.include_patterns.size());
            size_t includeCount = 0;
            std::vector<std::string> allFiles;
            // 遍历所有扫描目录，收集所有支持的文件
            for (const auto& directory : config_.scan.directories) {
                if (!utils::FileUtils::directoryExists(directory)) {
                    LOG_WARNING_FMT("目录不存在，跳过: %s", directory.c_str());
                    continue;
                }
                std::vector<std::string> files = utils::FileUtils::listFiles(
                    directory, [this](const std::string& path) { return isSupportedFileType(path); }, "", true);
                allFiles.insert(allFiles.end(), files.begin(), files.end());
            }
            // 对所有文件做shouldInclude判断
            for (const auto& filePath : allFiles) {
                if (!shouldInclude(filePath))
                    continue;
                SourceFileInfo fileInfo;
                fileInfo.path = filePath;
                fileInfo.relativePath = filePath;  // 可根据需要调整为相对路径
                fileInfo.size = utils::FileUtils::getFileSize(filePath);
                fileInfo.isHeader = utils::FileUtils::hasExtension(filePath, ".h") ||
                                    utils::FileUtils::hasExtension(filePath, ".hpp") ||
                                    utils::FileUtils::hasExtension(filePath, ".hxx");
                if (!readFileContent(filePath, fileInfo.content)) {
                    LOG_WARNING_FMT("无法读取文件内容: %s", filePath.c_str());
                    continue;
                }
                sourceFiles_.push_back(fileInfo);
                pathToIndex_[filePath] = sourceFiles_.size() - 1;
                ++includeCount;
                LOG_DEBUG_FMT("添加include文件: %s, 大小: %lu bytes", filePath.c_str(), fileInfo.size);
            }
            LOG_INFO_FMT("include_patterns count: %d, 扫描所有支持的源文件", config_.scan.include_patterns.size());
            LOG_INFO_FMT("共收集到 %zu 个include文件", includeCount);
            return core::ast_analyzer::makeSuccess(!sourceFiles_.empty());
        }

        std::vector<std::string> processedDirectories;

        // 遍历所有扫描目录
        for (const auto& directory : config_.scan.directories) {
            LOG_INFO_FMT("扫描目录: %s", directory.c_str());

            // 检查目录是否存在 - 改为警告而不是错误
            if (!utils::FileUtils::directoryExists(directory)) {
                LOG_WARNING_FMT("目录不存在，跳过: %s", directory.c_str());
                continue;  // 跳过不存在的目录，继续处理下一个
            }

            processedDirectories.push_back(directory);

            // 收集该目录下所有源文件
            std::vector<std::string> files = utils::FileUtils::listFiles(
                directory,
                [this](const std::string& path) { return isSupportedFileType(path) && !shouldExclude(path); }, "",
                true);

            LOG_INFO_FMT("在目录 %s 中找到 %lu 个源文件", directory.c_str(), files.size());

            // 处理找到的源文件
            for (const auto& filePath : files) {
                // 构建源文件信息
                SourceFileInfo fileInfo;
                fileInfo.path = filePath;
                fileInfo.relativePath = utils::FileUtils::getRelativePath(filePath, directory);
                fileInfo.size = utils::FileUtils::getFileSize(filePath);
                fileInfo.isHeader = utils::FileUtils::hasExtension(filePath, ".h") ||
                                    utils::FileUtils::hasExtension(filePath, ".hpp") ||
                                    utils::FileUtils::hasExtension(filePath, ".hxx");

                // 读取文件内容
                if (!readFileContent(filePath, fileInfo.content)) {
                    LOG_WARNING_FMT("无法读取文件内容: %s", filePath.c_str());
                    continue;
                }

                // 添加到源文件列表
                sourceFiles_.push_back(fileInfo);

                // 更新路径到索引的映射
                pathToIndex_[filePath] = sourceFiles_.size() - 1;

                LOG_DEBUG_FMT("添加源文件: %s, 大小: %lu bytes", filePath.c_str(), fileInfo.size);
            }
        }

        // 检查是否有任何目录被成功处理
        if (processedDirectories.empty()) {
            LOG_ERROR("所有扫描目录都不存在，无法继续");
            std::string allDirs = "";
            for (size_t i = 0; i < config_.scan.directories.size(); ++i) {
                if (i > 0) allDirs += ", ";
                allDirs += config_.scan.directories[i];
            }
            return core::ast_analyzer::makeError<bool>(
                core::ast_analyzer::ASTAnalyzerError::FILE_NOT_FOUND,
                "所有扫描目录都不存在: " + allDirs);
        }

        LOG_INFO_FMT("成功扫描了 %zu 个目录，共收集到 %lu 个源文件",
                     processedDirectories.size(), sourceFiles_.size());
        return core::ast_analyzer::makeSuccess(!sourceFiles_.empty());
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("收集源文件时发生异常: %s", e.what());
        return core::ast_analyzer::makeError<bool>(core::ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
                                                   std::string("收集源文件时发生异常: ") + e.what());
    }
}

const std::vector<SourceFileInfo>& SourceManager::getSourceFiles() const {
    return sourceFiles_;
}

size_t SourceManager::getSourceFileCount() const {
    return sourceFiles_.size();
}

const SourceFileInfo* SourceManager::getSourceFile(const std::string& path) const {
    auto it = pathToIndex_.find(path);
    if (it != pathToIndex_.end()) {
        return &sourceFiles_[it->second];
    }

    // 如果没有找到精确匹配，尝试使用规范化路径
    std::string normalizedPath = utils::FileUtils::normalizePath(path);
    for (auto& [existingPath, index] : pathToIndex_) {
        if (utils::FileUtils::normalizePath(existingPath) == normalizedPath) {
            return &sourceFiles_[index];
        }
    }

    return nullptr;
}

std::string SourceManager::globToRegex(const std::string& glob) const {
    std::string regex;
    regex.reserve(glob.size() * 2);

    for (size_t i = 0; i < glob.size(); ++i) {
        char c = glob[i];
        switch (c) {
            case '*':
                regex += ".*";
                break;
            case '?':
                regex += ".";
                break;
            case '.':
                regex += "\\.";
                break;
            case '+':
                regex += "\\+";
                break;
            case '^':
                regex += "\\^";
                break;
            case '$':
                regex += "\\$";
                break;
            case '(':
                regex += "\\(";
                break;
            case ')':
                regex += "\\)";
                break;
            case '[':
                regex += "\\[";
                break;
            case ']':
                regex += "\\]";
                break;
            case '{':
                regex += "\\{";
                break;
            case '}':
                regex += "\\}";
                break;
            case '|':
                regex += "\\|";
                break;
            case '\\':
                regex += "\\\\";
                break;
            default:
                regex += c;
                break;
        }
    }

    return regex;
}

// 仿照shouldExclude实现shouldInclude，支持glob匹配
bool SourceManager::shouldInclude(const std::string& path) const {
    for (const auto& pattern : config_.scan.include_patterns) {
        std::string regexPattern = globToRegex(pattern);
        try {
            std::regex regex(regexPattern, std::regex::extended);
            if (std::regex_search(path, regex)) {
                LOG_DEBUG_FMT("包含文件: %s, 匹配模式: %s", path.c_str(), pattern.c_str());
                return true;
            }
        } catch (const std::regex_error& e) {
            LOG_WARNING_FMT("include正则表达式错误，模式: %s, 错误: %s", pattern.c_str(), e.what());
            if (path.find(pattern) != std::string::npos) {
                LOG_DEBUG_FMT("包含文件: %s, 简单匹配模式: %s", path.c_str(), pattern.c_str());
                return true;
            }
        }
    }
    return false;
}

bool SourceManager::shouldExclude(const std::string& path) const {
    // 检查每个排除模式
    for (const auto& pattern : config_.scan.exclude_patterns) {
        // 将glob模式转换为正则表达式
        std::string regexPattern = globToRegex(pattern);

        try {
            std::regex regex(regexPattern, std::regex::extended);
            if (std::regex_search(path, regex)) {
                LOG_DEBUG_FMT("排除文件: %s, 匹配模式: %s", path.c_str(), pattern.c_str());
                return true;
            }
        } catch (const std::regex_error& e) {
            LOG_WARNING_FMT("正则表达式错误，模式: %s, 错误: %s", pattern.c_str(), e.what());
            // 如果正则表达式有错误，尝试简单的字符串匹配
            if (path.find(pattern) != std::string::npos) {
                LOG_DEBUG_FMT("排除文件: %s, 简单匹配模式: %s", path.c_str(), pattern.c_str());
                return true;
            }
        }
    }

    return false;
}

bool SourceManager::isSupportedFileType(const std::string& path) const {
    // 检查文件扩展名是否在支持的文件类型列表中
    for (const auto& extension : config_.scan.file_extensions) {
        if (utils::FileUtils::hasExtension(path, extension)) {
            return true;
        }
    }

    return false;
}

bool SourceManager::readFileContent(const std::string& path, std::string& content) const {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        LOG_ERROR_FMT("无法打开文件: %s", path.c_str());
        return false;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 读取文件内容
    content.resize(static_cast<size_t>(size));
    if (!file.read(&content[0], size)) {
        LOG_ERROR_FMT("读取文件失败: %s", path.c_str());
        return false;
    }

    return true;
}

}  // namespace source_manager
}  // namespace dlogcover
