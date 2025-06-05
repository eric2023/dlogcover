/**
 * @file cmake_parser.cpp
 * @brief CMake解析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/cmake_parser.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/string_utils.h>
#include <dlogcover/utils/file_utils.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cctype>

namespace dlogcover {
namespace utils {

using dlogcover::utils::split;
using dlogcover::utils::trim;

CMakeParser::CMakeParser() : verboseLogging_(false) {
    LOG_DEBUG("CMake解析器初始化");
    initializePatterns();
    
    // 设置常用的CMake内置变量
    setVariable("CMAKE_CURRENT_SOURCE_DIR", ".");
    setVariable("CMAKE_CURRENT_BINARY_DIR", ".");
    setVariable("CMAKE_SOURCE_DIR", ".");
    setVariable("CMAKE_BINARY_DIR", ".");
}

CMakeParser::~CMakeParser() {
    LOG_DEBUG("CMake解析器销毁");
}

void CMakeParser::initializePatterns() {
    // 注释模式：# 开头的行
    commentPatterns_.emplace_back(R"(^\s*#.*$)");
    
    // 命令模式：command(args)
    commandPatterns_.emplace_back(R"(^\s*(\w+)\s*\((.*)\)\s*$)");
}

Result<CMakeParseResult, CMakeParserError> CMakeParser::parse(const std::string& cmakeListsPath) {
    LOG_INFO_FMT("解析CMakeLists.txt文件: %s", cmakeListsPath.c_str());
    
    // 检查文件是否存在
    if (!std::filesystem::exists(cmakeListsPath)) {
        return makeError<CMakeParseResult, CMakeParserError>(CMakeParserError::FILE_NOT_FOUND, 
                                                            "CMakeLists.txt文件不存在: " + cmakeListsPath);
    }
    
    // 读取文件内容
    std::ifstream file(cmakeListsPath);
    if (!file.is_open()) {
        return makeError<CMakeParseResult, CMakeParserError>(CMakeParserError::FILE_READ_ERROR, 
                                                            "无法打开CMakeLists.txt文件: " + cmakeListsPath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // 获取源码目录
    std::string sourceDir = std::filesystem::path(cmakeListsPath).parent_path().string();
    if (sourceDir.empty()) {
        sourceDir = ".";
    }
    
    // 解析内容
    auto result = parseContent(content, sourceDir);
    if (result.hasError()) {
        return result;
    }
    
    // 设置CMakeLists.txt路径
    result.value().cmakeListsPath = cmakeListsPath;
    result.value().sourceDir = sourceDir;
    
    LOG_INFO_FMT("CMakeLists.txt解析完成: %s", cmakeListsPath.c_str());
    return result;
}

Result<CMakeParseResult, CMakeParserError> CMakeParser::parseContent(const std::string& content, 
                                                                           const std::string& sourceDir) {
    LOG_DEBUG("开始解析CMake内容");
    
    CMakeParseResult result;
    result.sourceDir = sourceDir;
    
    // 设置源码目录相关变量
    setVariable("CMAKE_CURRENT_SOURCE_DIR", sourceDir);
    setVariable("CMAKE_SOURCE_DIR", sourceDir);
    
    // 预处理内容
    std::string processedContent = preprocessContent(content);
    
    // 按行解析
    std::istringstream stream(processedContent);
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(stream, line)) {
        lineNumber++;
        
        // 跳过空行
        if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace)) {
            continue;
        }
        
        // 解析命令
        if (!parseCommand(line, result)) {
            logWarning("解析命令失败，行号: " + std::to_string(lineNumber) + ", 内容: " + line);
        }
    }
    
    LOG_DEBUG("CMake内容解析完成");
    return makeSuccess<CMakeParseResult, CMakeParserError>(std::move(result));
}

std::string CMakeParser::preprocessContent(const std::string& content) {
    // 移除注释
    std::string processed = removeComments(content);
    
    // 处理多行命令（简单实现）
    std::string result;
    std::istringstream stream(processed);
    std::string line;
    std::string currentCommand;
    
    while (std::getline(stream, line)) {
        // 去除首尾空白
        line = trim(line);
        
        if (line.empty()) {
            continue;
        }
        
        // 检查是否为命令的延续
        if (!currentCommand.empty()) {
            currentCommand += " " + line;
            
            // 检查是否结束（简单的括号匹配）
            int openCount = 0, closeCount = 0;
            for (char c : currentCommand) {
                if (c == '(') openCount++;
                else if (c == ')') closeCount++;
            }
            
            if (openCount == closeCount && openCount > 0) {
                result += currentCommand + "\n";
                currentCommand.clear();
            }
        } else {
            // 检查是否为新命令的开始
            if (line.find('(') != std::string::npos) {
                currentCommand = line;
                
                // 检查是否在同一行结束
                int openCount = 0, closeCount = 0;
                for (char c : line) {
                    if (c == '(') openCount++;
                    else if (c == ')') closeCount++;
                }
                
                if (openCount == closeCount && openCount > 0) {
                    result += line + "\n";
                    currentCommand.clear();
                }
            } else {
                result += line + "\n";
            }
        }
    }
    
    // 处理未完成的命令
    if (!currentCommand.empty()) {
        result += currentCommand + "\n";
    }
    
    return result;
}

std::string CMakeParser::removeComments(const std::string& content) {
    std::string result;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // 查找注释位置
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            // 检查是否在字符串内
            bool inString = false;
            char stringChar = '\0';
            
            for (size_t i = 0; i < commentPos; ++i) {
                if (!inString && (line[i] == '"' || line[i] == '\'')) {
                    inString = true;
                    stringChar = line[i];
                } else if (inString && line[i] == stringChar && (i == 0 || line[i-1] != '\\')) {
                    inString = false;
                }
            }
            
            if (!inString) {
                line = line.substr(0, commentPos);
            }
        }
        
        result += line + "\n";
    }
    
    return result;
}

bool CMakeParser::parseCommand(const std::string& line, CMakeParseResult& result) {
    // 使用正则表达式匹配命令
    for (const auto& pattern : commandPatterns_) {
        std::smatch match;
        if (std::regex_match(line, match, pattern)) {
            std::string command = match[1].str();
            std::string argsStr = match[2].str();
            
            // 转换为小写进行比较
            std::transform(command.begin(), command.end(), command.begin(), ::tolower);
            
            // 分割参数
            std::vector<std::string> args = splitCommandArgs(argsStr);
            
            logDebug("解析命令: " + command + ", 参数数量: " + std::to_string(args.size()));
            
            // 根据命令类型进行处理
            if (command == "project") {
                parseProjectCommand(args, result);
            } else if (command == "set") {
                parseSetCommand(args, result);
            } else if (command == "include_directories") {
                parseIncludeDirectoriesCommand(args, result);
            } else if (command == "add_definitions") {
                parseAddDefinitionsCommand(args, result);
            } else if (command == "add_executable") {
                parseAddExecutableCommand(args, result);
            } else if (command == "add_library") {
                parseAddLibraryCommand(args, result);
            } else if (command == "target_include_directories") {
                parseTargetIncludeDirectoriesCommand(args, result);
            } else if (command == "target_compile_definitions") {
                parseTargetCompileDefinitionsCommand(args, result);
            } else if (command == "target_compile_options") {
                parseTargetCompileOptionsCommand(args, result);
            } else if (command == "target_link_libraries") {
                parseTargetLinkLibrariesCommand(args, result);
            } else if (command == "find_package") {
                parseFindPackageCommand(args, result);
            } else {
                logDebug("未处理的命令: " + command);
            }
            
            return true;
        }
    }
    
    return false;
}

void CMakeParser::parseProjectCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.empty()) {
        logWarning("project命令缺少项目名称");
        return;
    }
    
    result.projectName = args[0];
    logDebug("设置项目名称: " + result.projectName);
    
    // 解析其他参数
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "VERSION" && i + 1 < args.size()) {
            result.projectVersion = args[i + 1];
            logDebug("设置项目版本: " + result.projectVersion);
            ++i;
        } else if (args[i] == "LANGUAGES") {
            // 处理语言列表
            for (size_t j = i + 1; j < args.size(); ++j) {
                if (args[j] == "CXX" || args[j] == "C") {
                    logDebug("项目语言: " + args[j]);
                }
            }
            break;
        }
    }
}

void CMakeParser::parseSetCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.size() < 2) {
        logWarning("set命令参数不足");
        return;
    }
    
    std::string varName = args[0];
    std::string varValue = args[1];
    
    // 展开变量
    varValue = expandVariables(varValue, result);
    
    // 设置变量
    setVariable(varName, varValue);
    
    // 处理特殊变量
    if (varName == "CMAKE_CXX_STANDARD") {
        result.cxxStandard = varValue;
        logDebug("设置C++标准: " + varValue);
    } else if (varName == "CMAKE_C_STANDARD") {
        result.cStandard = varValue;
        logDebug("设置C标准: " + varValue);
    } else if (varName == "CMAKE_CXX_FLAGS" || varName == "CMAKE_C_FLAGS") {
        // 解析编译选项
        std::vector<std::string> flags = split(varValue, " ");
        for (const auto& flag : flags) {
            if (!flag.empty()) {
                result.compileOptions.push_back(flag);
            }
        }
        logDebug("添加编译选项: " + varValue);
    }
    
    logDebug("设置变量: " + varName + " = " + varValue);
}

void CMakeParser::parseIncludeDirectoriesCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    for (const auto& arg : args) {
        std::string path = expandVariables(arg, result);
        path = normalizePath(path, result.sourceDir);
        result.includeDirectories.push_back(path);
        logDebug("添加包含目录: " + path);
    }
}

void CMakeParser::parseAddDefinitionsCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    for (const auto& arg : args) {
        std::string def = expandVariables(arg, result);
        result.compileDefinitions.push_back(def);
        logDebug("添加编译定义: " + def);
    }
}

void CMakeParser::parseAddExecutableCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.empty()) {
        logWarning("add_executable命令缺少目标名称");
        return;
    }
    
    std::string targetName = args[0];
    CMakeTarget target(targetName, "EXECUTABLE");
    
    // 添加源文件
    for (size_t i = 1; i < args.size(); ++i) {
        std::string source = expandVariables(args[i], result);
        source = normalizePath(source, result.sourceDir);
        target.sources.push_back(source);
    }
    
    result.targets[targetName] = target;
    logDebug("添加可执行目标: " + targetName);
}

void CMakeParser::parseAddLibraryCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.empty()) {
        logWarning("add_library命令缺少目标名称");
        return;
    }
    
    std::string targetName = args[0];
    CMakeTarget target(targetName, "LIBRARY");
    
    // 跳过库类型参数（STATIC, SHARED, MODULE等）
    size_t startIdx = 1;
    if (args.size() > 1) {
        std::string type = args[1];
        std::transform(type.begin(), type.end(), type.begin(), ::toupper);
        if (type == "STATIC" || type == "SHARED" || type == "MODULE") {
            target.type = type + "_LIBRARY";
            startIdx = 2;
        }
    }
    
    // 添加源文件
    for (size_t i = startIdx; i < args.size(); ++i) {
        std::string source = expandVariables(args[i], result);
        source = normalizePath(source, result.sourceDir);
        target.sources.push_back(source);
    }
    
    result.targets[targetName] = target;
    logDebug("添加库目标: " + targetName);
}

void CMakeParser::parseTargetIncludeDirectoriesCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.size() < 2) {
        logWarning("target_include_directories命令参数不足");
        return;
    }
    
    std::string targetName = args[0];
    auto it = result.targets.find(targetName);
    if (it == result.targets.end()) {
        logWarning("目标不存在: " + targetName);
        return;
    }
    
    // 跳过访问修饰符（PUBLIC, PRIVATE, INTERFACE）
    size_t startIdx = 1;
    if (args.size() > 1) {
        std::string modifier = args[1];
        std::transform(modifier.begin(), modifier.end(), modifier.begin(), ::toupper);
        if (modifier == "PUBLIC" || modifier == "PRIVATE" || modifier == "INTERFACE") {
            startIdx = 2;
        }
    }
    
    for (size_t i = startIdx; i < args.size(); ++i) {
        std::string path = expandVariables(args[i], result);
        path = normalizePath(path, result.sourceDir);
        it->second.includeDirectories.push_back(path);
        logDebug("为目标 " + targetName + " 添加包含目录: " + path);
    }
}

void CMakeParser::parseTargetCompileDefinitionsCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.size() < 2) {
        logWarning("target_compile_definitions命令参数不足");
        return;
    }
    
    std::string targetName = args[0];
    auto it = result.targets.find(targetName);
    if (it == result.targets.end()) {
        logWarning("目标不存在: " + targetName);
        return;
    }
    
    // 跳过访问修饰符
    size_t startIdx = 1;
    if (args.size() > 1) {
        std::string modifier = args[1];
        std::transform(modifier.begin(), modifier.end(), modifier.begin(), ::toupper);
        if (modifier == "PUBLIC" || modifier == "PRIVATE" || modifier == "INTERFACE") {
            startIdx = 2;
        }
    }
    
    for (size_t i = startIdx; i < args.size(); ++i) {
        std::string def = expandVariables(args[i], result);
        it->second.compileDefinitions.push_back(def);
        logDebug("为目标 " + targetName + " 添加编译定义: " + def);
    }
}

void CMakeParser::parseTargetCompileOptionsCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.size() < 2) {
        logWarning("target_compile_options命令参数不足");
        return;
    }
    
    std::string targetName = args[0];
    auto it = result.targets.find(targetName);
    if (it == result.targets.end()) {
        logWarning("目标不存在: " + targetName);
        return;
    }
    
    // 跳过访问修饰符
    size_t startIdx = 1;
    if (args.size() > 1) {
        std::string modifier = args[1];
        std::transform(modifier.begin(), modifier.end(), modifier.begin(), ::toupper);
        if (modifier == "PUBLIC" || modifier == "PRIVATE" || modifier == "INTERFACE") {
            startIdx = 2;
        }
    }
    
    for (size_t i = startIdx; i < args.size(); ++i) {
        std::string option = expandVariables(args[i], result);
        it->second.compileOptions.push_back(option);
        logDebug("为目标 " + targetName + " 添加编译选项: " + option);
    }
}

void CMakeParser::parseTargetLinkLibrariesCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.size() < 2) {
        logWarning("target_link_libraries命令参数不足");
        return;
    }
    
    std::string targetName = args[0];
    auto it = result.targets.find(targetName);
    if (it == result.targets.end()) {
        logWarning("目标不存在: " + targetName);
        return;
    }
    
    // 跳过访问修饰符
    size_t startIdx = 1;
    if (args.size() > 1) {
        std::string modifier = args[1];
        std::transform(modifier.begin(), modifier.end(), modifier.begin(), ::toupper);
        if (modifier == "PUBLIC" || modifier == "PRIVATE" || modifier == "INTERFACE") {
            startIdx = 2;
        }
    }
    
    for (size_t i = startIdx; i < args.size(); ++i) {
        std::string lib = expandVariables(args[i], result);
        it->second.linkLibraries.push_back(lib);
        logDebug("为目标 " + targetName + " 添加链接库: " + lib);
    }
}

void CMakeParser::parseFindPackageCommand(const std::vector<std::string>& args, CMakeParseResult& result) {
    if (args.empty()) {
        logWarning("find_package命令缺少包名称");
        return;
    }
    
    std::string packageName = args[0];
    logDebug("查找包: " + packageName);
    
    // 设置一些常见包的变量（简化实现）
    if (packageName == "LLVM") {
        setVariable("LLVM_FOUND", "TRUE");
        setVariable("LLVM_INCLUDE_DIRS", "/usr/lib/llvm-13/include");
        setVariable("LLVM_LIBRARY_DIRS", "/usr/lib/llvm-13/lib");
    } else if (packageName == "Clang") {
        setVariable("Clang_FOUND", "TRUE");
    }
}

std::vector<std::string> CMakeParser::splitCommandArgs(const std::string& commandLine) {
    std::vector<std::string> args;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';
    
    for (size_t i = 0; i < commandLine.length(); ++i) {
        char c = commandLine[i];
        
        if (!inQuotes) {
            if (c == '"' || c == '\'') {
                inQuotes = true;
                quoteChar = c;
            } else if (std::isspace(c)) {
                if (!current.empty()) {
                    args.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        } else {
            if (c == quoteChar) {
                inQuotes = false;
                quoteChar = '\0';
            } else {
                current += c;
            }
        }
    }
    
    if (!current.empty()) {
        args.push_back(current);
    }
    
    return args;
}

std::string CMakeParser::expandVariables(const std::string& text, const CMakeParseResult& result) {
    std::string expanded = text;
    
    // 简单的变量展开实现
    std::regex varPattern(R"(\$\{([^}]+)\})");
    std::smatch match;
    
    while (std::regex_search(expanded, match, varPattern)) {
        std::string varName = match[1].str();
        std::string varValue = getVariable(varName);
        
        if (varValue.empty()) {
            // 尝试从解析结果中获取
            auto it = result.variables.find(varName);
            if (it != result.variables.end()) {
                varValue = it->second.value;
            }
        }
        
        expanded.replace(match.position(), match.length(), varValue);
    }
    
    return expanded;
}

std::string CMakeParser::normalizePath(const std::string& path, const std::string& sourceDir) {
    if (isAbsolutePath(path)) {
        return path;
    }
    
    // 相对路径，相对于源码目录
    std::filesystem::path fullPath = std::filesystem::path(sourceDir) / path;
    return fullPath.lexically_normal().string();
}

bool CMakeParser::isAbsolutePath(const std::string& path) {
    return std::filesystem::path(path).is_absolute();
}

void CMakeParser::setVerboseLogging(bool enabled) {
    verboseLogging_ = enabled;
}

void CMakeParser::setVariable(const std::string& name, const std::string& value) {
    variables_[name] = value;
}

std::string CMakeParser::getVariable(const std::string& name) const {
    auto it = variables_.find(name);
    return (it != variables_.end()) ? it->second : "";
}

void CMakeParser::clearVariables() {
    variables_.clear();
}

void CMakeParser::logDebug(const std::string& message) {
    if (verboseLogging_) {
        LOG_DEBUG_FMT("[CMakeParser] %s", message.c_str());
    }
}

void CMakeParser::logWarning(const std::string& message) {
    LOG_WARNING_FMT("[CMakeParser] %s", message.c_str());
}

void CMakeParser::logError(const std::string& message) {
    LOG_ERROR_FMT("[CMakeParser] %s", message.c_str());
}

// CMakeParseResult 方法实现
std::vector<std::string> CMakeParseResult::getAllCompilerArgs() const {
    std::vector<std::string> args;
    
    // 添加C++标准
    if (!cxxStandard.empty()) {
        args.push_back("-std=c++" + cxxStandard);
    }
    
    // 添加包含目录
    for (const auto& dir : includeDirectories) {
        args.push_back("-I" + dir);
    }
    
    // 添加编译定义
    for (const auto& def : compileDefinitions) {
        // 过滤掉以 '-' 开头的编译选项，这些不应该是宏定义
        if (!def.empty() && def[0] != '-') {
            if (def.find('=') != std::string::npos) {
                args.push_back("-D" + def);
            } else {
                args.push_back("-D" + def);
            }
        } else {
            // 如果是以 '-' 开头的，直接作为编译选项添加
            args.push_back(def);
        }
    }
    
    // 添加编译选项
    for (const auto& option : compileOptions) {
        args.push_back(option);
    }
    
    return args;
}

std::vector<std::string> CMakeParseResult::getTargetCompilerArgs(const std::string& targetName) const {
    std::vector<std::string> args = getAllCompilerArgs();
    
    auto it = targets.find(targetName);
    if (it != targets.end()) {
        const CMakeTarget& target = it->second;
        
        // 添加目标特定的包含目录
        for (const auto& dir : target.includeDirectories) {
            args.push_back("-I" + dir);
        }
        
        // 添加目标特定的编译定义
        for (const auto& def : target.compileDefinitions) {
            // 过滤掉以 '-' 开头的编译选项，这些不应该是宏定义
            if (!def.empty() && def[0] != '-') {
                args.push_back("-D" + def);
            } else {
                // 如果是以 '-' 开头的，直接作为编译选项添加
                args.push_back(def);
            }
        }
        
        // 添加目标特定的编译选项
        for (const auto& option : target.compileOptions) {
            args.push_back(option);
        }
    }
    
    return args;
}

bool CMakeParseResult::isValid() const {
    return !projectName.empty() || !targets.empty();
}

void CMakeParseResult::clear() {
    projectName.clear();
    projectVersion.clear();
    cxxStandard.clear();
    cStandard.clear();
    includeDirectories.clear();
    compileDefinitions.clear();
    compileOptions.clear();
    linkDirectories.clear();
    linkLibraries.clear();
    variables.clear();
    targets.clear();
    sourceDir.clear();
    binaryDir.clear();
    cmakeListsPath.clear();
}

} // namespace utils
} // namespace dlogcover 