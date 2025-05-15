/**
 * @file options.cpp
 * @brief 命令行选项实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/options.h>
#include <dlogcover/utils/file_utils.h>

#include <filesystem>

namespace dlogcover {
namespace cli {

bool Options::isValid() const {
    // 检查扫描目录是否存在
    if (!directoryPath.empty() && !utils::FileUtils::directoryExists(directoryPath)) {
        return false;
    }

    // 检查配置文件是否存在
    if (!configPath.empty() && !utils::FileUtils::fileExists(configPath)) {
        return false;
    }

    // 检查输出路径的父目录是否存在
    if (!outputPath.empty()) {
        std::filesystem::path outputFilePath(outputPath);
        std::filesystem::path parentPath = outputFilePath.parent_path();
        if (!parentPath.empty() && !utils::FileUtils::directoryExists(parentPath.string())) {
            return false;
        }
    }

    return true;
}

}  // namespace cli
}  // namespace dlogcover
