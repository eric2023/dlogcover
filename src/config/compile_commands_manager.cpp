/**
 * @file compile_commands_manager.cpp
 * @brief 编译命令管理器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/compile_commands_manager.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/file_utils.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace dlogcover {
namespace config {

CompileCommandsManager::CompileCommandsManager() {
    LOG_DEBUG("编译命令管理器初始化");
}

CompileCommandsManager::~CompileCommandsManager() {
    LOG_DEBUG("编译命令管理器销毁");
}

bool CompileCommandsManager::generateCompileCommands(const std::string& project_dir, 
                                                    const std::string& build_dir,
                                                    const std::vector<std::string>& cmake_args) {
    LOG_INFO_FMT("开始生成compile_commands.json: 项目目录=%s, 构建目录=%s", 
                 project_dir.c_str(), build_dir.c_str());

    // 检查CMake是否可用
    if (!isCMakeAvailable()) {
        setError("CMake不可用，请确保已安装CMake并添加到PATH");
        return false;
    }

    // 检查项目目录是否存在CMakeLists.txt
    std::string cmake_lists_path = project_dir + "/CMakeLists.txt";
    if (!std::filesystem::exists(cmake_lists_path)) {
        setError("项目目录中未找到CMakeLists.txt文件: " + cmake_lists_path);
        return false;
    }

    // 创建构建目录
    try {
        std::filesystem::create_directories(build_dir);
        LOG_DEBUG_FMT("创建构建目录: %s", build_dir.c_str());
    } catch (const std::exception& e) {
        setError("无法创建构建目录: " + std::string(e.what()));
        return false;
    }

    // 运行CMake
    return runCMake(project_dir, build_dir, cmake_args);
}

bool CompileCommandsManager::parseCompileCommands(const std::string& file_path) {
    LOG_INFO_FMT("开始解析compile_commands.json: %s", file_path.c_str());

    // 清除之前的数据
    clear();

    // 检查文件是否存在
    if (!std::filesystem::exists(file_path)) {
        setError("compile_commands.json文件不存在: " + file_path);
        return false;
    }

    try {
        // 读取JSON文件
        std::ifstream file(file_path);
        if (!file.is_open()) {
            setError("无法打开compile_commands.json文件: " + file_path);
            return false;
        }

        nlohmann::json json_data;
        file >> json_data;

        if (!json_data.is_array()) {
            setError("compile_commands.json格式错误：根元素不是数组");
            return false;
        }

        // 解析每个编译条目
        int parsed_count = 0;
        for (const auto& entry : json_data) {
            if (!entry.is_object()) {
                LOG_WARNING("跳过无效的编译条目（不是对象）");
                continue;
            }

            // 检查必需字段
            if (entry.find("file") == entry.end() || entry.find("command") == entry.end() || entry.find("directory") == entry.end()) {
                LOG_WARNING("跳过无效的编译条目（缺少必需字段）");
                continue;
            }

            CompileInfo compile_info;
            compile_info.file = entry["file"].get<std::string>();
            compile_info.command = entry["command"].get<std::string>();
            compile_info.directory = entry["directory"].get<std::string>();

            // 解析编译命令
            if (parseCompileCommand(compile_info.command, compile_info)) {
                // 标准化文件路径
                std::string normalized_path = normalizePath(compile_info.file);
                compile_info_map_[normalized_path] = compile_info;
                parsed_count++;
            }
        }

        LOG_INFO_FMT("成功解析compile_commands.json，共解析 %d 个文件", parsed_count);
        return parsed_count > 0;

    } catch (const std::exception& e) {
        setError("解析compile_commands.json失败: " + std::string(e.what()));
        return false;
    }
}

CompileInfo CompileCommandsManager::getCompileInfoForFile(const std::string& file_path) const {
    std::string normalized_path = normalizePath(file_path);
    
    auto it = compile_info_map_.find(normalized_path);
    if (it != compile_info_map_.end()) {
        return it->second;
    }

    // 如果直接查找失败，尝试匹配文件名
    std::string filename = std::filesystem::path(file_path).filename().string();
    for (const auto& pair : compile_info_map_) {
        if (std::filesystem::path(pair.first).filename().string() == filename) {
            LOG_DEBUG_FMT("通过文件名匹配找到编译信息: %s -> %s", 
                         file_path.c_str(), pair.first.c_str());
            return pair.second;
        }
    }

    LOG_WARNING_FMT("未找到文件的编译信息: %s", file_path.c_str());
    return CompileInfo{}; // 返回空的编译信息
}

bool CompileCommandsManager::isCompileCommandsValid(const std::string& file_path) const {
    if (!std::filesystem::exists(file_path)) {
        return false;
    }

    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json json_data;
        file >> json_data;

        return json_data.is_array() && !json_data.empty();
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<std::string> CompileCommandsManager::getAllFiles() const {
    std::vector<std::string> files;
    files.reserve(compile_info_map_.size());
    
    for (const auto& pair : compile_info_map_) {
        files.push_back(pair.first);
    }
    
    return files;
}

void CompileCommandsManager::clear() {
    compile_info_map_.clear();
    error_.clear();
}

bool CompileCommandsManager::parseCompileCommand(const std::string& command, CompileInfo& compile_info) {
    // 使用正则表达式解析编译命令中的包含路径和宏定义
    std::regex include_regex(R"(-I\s*([^\s]+))");
    std::regex define_regex(R"(-D\s*([^\s]+))");
    std::regex flag_regex(R"(-[a-zA-Z][^\s]*)");

    std::sregex_iterator iter, end;

    // 解析包含路径
    iter = std::sregex_iterator(command.begin(), command.end(), include_regex);
    for (; iter != end; ++iter) {
        std::string include_path = (*iter)[1].str();
        if (!include_path.empty()) {
            compile_info.include_paths.push_back(include_path);
        }
    }

    // 解析宏定义
    iter = std::sregex_iterator(command.begin(), command.end(), define_regex);
    for (; iter != end; ++iter) {
        std::string define = (*iter)[1].str();
        if (!define.empty()) {
            compile_info.defines.push_back(define);
        }
    }

    // 解析编译标志
    iter = std::sregex_iterator(command.begin(), command.end(), flag_regex);
    for (; iter != end; ++iter) {
        std::string flag = (*iter)[0].str();
        if (!flag.empty() && flag != "-I" && flag != "-D") {
            compile_info.flags.push_back(flag);
        }
    }

    LOG_DEBUG_FMT("解析编译命令: 文件=%s, 包含路径=%zu个, 宏定义=%zu个, 标志=%zu个",
                  compile_info.file.c_str(), 
                  compile_info.include_paths.size(),
                  compile_info.defines.size(),
                  compile_info.flags.size());

    return true;
}

std::string CompileCommandsManager::normalizePath(const std::string& file_path) const {
    try {
        return std::filesystem::canonical(file_path).string();
    } catch (const std::exception&) {
        // 如果无法规范化（文件不存在等），返回绝对路径
        try {
            return std::filesystem::absolute(file_path).string();
        } catch (const std::exception&) {
            return file_path; // 最后的备选方案
        }
    }
}

bool CompileCommandsManager::isCMakeAvailable() const {
    // 尝试运行cmake --version来检查是否可用
    int result = std::system("cmake --version > /dev/null 2>&1");
    return result == 0;
}

bool CompileCommandsManager::runCMake(const std::string& project_dir, 
                                     const std::string& build_dir,
                                     const std::vector<std::string>& cmake_args) {
    // 构建CMake命令
    std::ostringstream cmd;
    cmd << "cd \"" << project_dir << "\" && cmake";

    // 添加构建目录参数
    cmd << " -B \"" << build_dir << "\"";
    
    // 添加导出编译命令的参数
    cmd << " -DCMAKE_EXPORT_COMPILE_COMMANDS=1";
    
    // 添加用户指定的CMake参数
    for (const auto& arg : cmake_args) {
        cmd << " " << arg;
    }
    
    // 添加源目录
    cmd << " .";
    
    std::string command = cmd.str();
    LOG_INFO_FMT("执行CMake命令: %s", command.c_str());

    // 执行命令
    int result = std::system(command.c_str());
    if (result != 0) {
        setError("CMake执行失败，返回码: " + std::to_string(result));
        return false;
    }

    // 检查是否生成了compile_commands.json
    std::string compile_commands_path = build_dir + "/compile_commands.json";
    if (!std::filesystem::exists(compile_commands_path)) {
        setError("CMake执行成功但未生成compile_commands.json文件");
        return false;
    }

    LOG_INFO_FMT("成功生成compile_commands.json: %s", compile_commands_path.c_str());
    return true;
}

void CompileCommandsManager::setError(const std::string& message) {
    error_ = message;
    LOG_ERROR_FMT("编译命令管理器错误: %s", message.c_str());
}

std::vector<std::string> CompileCommandsManager::getCompilerArgs(const std::string& filePath) const {
    std::vector<std::string> args;
    
    // 首先尝试从 compile_commands.json 获取精确匹配
    if (!compile_info_map_.empty()) {
        std::string normalizedPath = normalizePath(filePath);
        
        for (const auto& entry : compile_info_map_) {
            if (entry.first == normalizedPath) {
                if (!entry.second.command.empty()) {
                    args = parseCompilerCommand(entry.second.command);
                    LOG_DEBUG_FMT("找到精确匹配的编译命令: %s", filePath.c_str());
                    return args;
                }
            }
        }
        
        // 如果精确匹配失败，尝试查找同名文件的编译参数
        std::filesystem::path inputPath(filePath);
        std::string fileName = inputPath.filename().string();
        
        LOG_INFO_FMT("文件 %s 不在 compile_commands.json 中，尝试查找同名文件: %s", 
                     filePath.c_str(), fileName.c_str());
        
        for (const auto& entry : compile_info_map_) {
            std::filesystem::path entryPath(entry.first);
            if (entryPath.filename().string() == fileName) {
                if (!entry.second.command.empty()) {
                    args = parseCompilerCommand(entry.second.command);
                    LOG_INFO_FMT("找到同名文件的编译命令: %s -> %s", 
                                filePath.c_str(), entry.first.c_str());
                    return args;
                }
            }
        }
    }
    
    // 如果都找不到，提供基础的回退编译参数
    LOG_WARNING_FMT("文件 %s 及其同名文件都不在 compile_commands.json 中，使用基础编译参数", 
                    filePath.c_str());
    
    // 基础 C++ 编译参数
    args = {
        "-std=c++14",
        "-fPIC",
        "-g"
    };
    
    // 添加基础系统包含路径
    std::vector<std::string> basicIncludes = {
        "/usr/include",
        "/usr/include/c++/12",
        "/usr/include/x86_64-linux-gnu/qt5",
        "/usr/include/x86_64-linux-gnu/qt5/QtCore",
        "/usr/include/x86_64-linux-gnu/qt5/QtGui",
        "/usr/include/x86_64-linux-gnu/qt5/QtWidgets"
    };
    
    for (const auto& include : basicIncludes) {
        args.push_back("-I" + include);
    }
    
    // 添加基础宏定义
    args.push_back("-DQT_CORE_LIB");
    args.push_back("-DQT_GUI_LIB");
    args.push_back("-DQT_WIDGETS_LIB");
    
    return args;
}

std::string CompileCommandsManager::getProjectDirectory(const std::string& filePath) const {
    // 从文件路径推断项目目录
    std::filesystem::path path(filePath);
    std::filesystem::path current = path.parent_path();
    
    // 向上查找包含 CMakeLists.txt 的目录
    while (!current.empty() && current != current.root_path()) {
        if (std::filesystem::exists(current / "CMakeLists.txt")) {
            return current.string();
        }
        current = current.parent_path();
    }
    
    return "";
}

std::vector<std::string> CompileCommandsManager::parseCompilerCommand(const std::string& command) const {
    std::vector<std::string> args;
    
    // 简单的命令行解析 - 按空格分割，但要处理引号
    std::string current_arg;
    bool in_quotes = false;
    bool escape_next = false;
    
    for (size_t i = 0; i < command.length(); ++i) {
        char c = command[i];
        
        if (escape_next) {
            current_arg += c;
            escape_next = false;
            continue;
        }
        
        if (c == '\\') {
            escape_next = true;
            continue;
        }
        
        if (c == '"' || c == '\'') {
            in_quotes = !in_quotes;
            continue;
        }
        
        if (c == ' ' && !in_quotes) {
            if (!current_arg.empty()) {
                args.push_back(current_arg);
                current_arg.clear();
            }
        } else {
            current_arg += c;
        }
    }
    
    if (!current_arg.empty()) {
        args.push_back(current_arg);
    }
    
    // 过滤掉编译器名称和输出文件参数，只保留编译选项
    std::vector<std::string> filtered_args;
    bool skip_next = false;
    
    for (size_t i = 1; i < args.size(); ++i) { // 跳过第一个参数（编译器名称）
        if (skip_next) {
            skip_next = false;
            continue;
        }
        
        const std::string& arg = args[i];
        
        // 跳过输出文件相关参数
        if (arg == "-o" || arg == "-c") {
            if (arg == "-o") {
                skip_next = true; // 跳过下一个参数（输出文件名）
            }
            continue;
        }
        
        // 跳过源文件参数（通常是最后一个参数）
        if (i == args.size() - 1) {
            size_t len = arg.length();
            if ((len > 4 && arg.substr(len - 4) == ".cpp") ||
                (len > 3 && arg.substr(len - 3) == ".cc") ||
                (len > 4 && arg.substr(len - 4) == ".cxx") ||
                (len > 2 && arg.substr(len - 2) == ".c")) {
                continue;
            }
        }
        
        filtered_args.push_back(arg);
    }
    
    return filtered_args;
}

} // namespace config
} // namespace dlogcover 