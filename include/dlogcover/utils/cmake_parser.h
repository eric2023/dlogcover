/**
 * @file cmake_parser.h
 * @brief CMake解析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_CMAKE_PARSER_H
#define DLOGCOVER_UTILS_CMAKE_PARSER_H

#include <dlogcover/utils/cmake_types.h>
#include <dlogcover/common/result.h>

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <unordered_map>

namespace dlogcover {
namespace utils {

/**
 * @brief CMake解析器类
 * 
 * 负责解析CMakeLists.txt文件，提取编译相关的配置信息
 */
class CMakeParser {
public:
    /**
     * @brief 构造函数
     */
    CMakeParser();

    /**
     * @brief 析构函数
     */
    ~CMakeParser();

    /**
     * @brief 解析CMakeLists.txt文件
     * @param cmakeListsPath CMakeLists.txt文件路径
     * @return 解析结果
     */
    Result<CMakeParseResult, CMakeParserError> parse(const std::string& cmakeListsPath);

    /**
     * @brief 解析CMakeLists.txt内容
     * @param content CMakeLists.txt文件内容
     * @param sourceDir 源码目录路径
     * @return 解析结果
     */
    Result<CMakeParseResult, CMakeParserError> parseContent(const std::string& content, 
                                                            const std::string& sourceDir = ".");

    /**
     * @brief 设置是否启用详细日志
     * @param enabled 是否启用
     */
    void setVerboseLogging(bool enabled);

    /**
     * @brief 设置变量值
     * @param name 变量名
     * @param value 变量值
     */
    void setVariable(const std::string& name, const std::string& value);

    /**
     * @brief 获取变量值
     * @param name 变量名
     * @return 变量值，如果不存在返回空字符串
     */
    std::string getVariable(const std::string& name) const;

    /**
     * @brief 清空所有变量
     */
    void clearVariables();

private:
    bool verboseLogging_;                                    ///< 是否启用详细日志
    std::unordered_map<std::string, std::string> variables_; ///< 变量映射表
    std::vector<std::regex> commentPatterns_;                ///< 注释模式
    std::vector<std::regex> commandPatterns_;                ///< 命令模式

    /**
     * @brief 初始化正则表达式模式
     */
    void initializePatterns();

    /**
     * @brief 预处理CMake内容
     * @param content 原始内容
     * @return 预处理后的内容
     */
    std::string preprocessContent(const std::string& content);

    /**
     * @brief 移除注释
     * @param content 内容
     * @return 移除注释后的内容
     */
    std::string removeComments(const std::string& content);

    /**
     * @brief 解析CMake命令
     * @param line 命令行
     * @param result 解析结果
     * @return 是否解析成功
     */
    bool parseCommand(const std::string& line, CMakeParseResult& result);

    /**
     * @brief 解析project命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseProjectCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析set命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseSetCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析include_directories命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseIncludeDirectoriesCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析add_definitions命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseAddDefinitionsCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析add_executable命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseAddExecutableCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析add_library命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseAddLibraryCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析target_include_directories命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseTargetIncludeDirectoriesCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析target_compile_definitions命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseTargetCompileDefinitionsCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析target_compile_options命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseTargetCompileOptionsCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析target_link_libraries命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseTargetLinkLibrariesCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 解析find_package命令
     * @param args 参数列表
     * @param result 解析结果
     */
    void parseFindPackageCommand(const std::vector<std::string>& args, CMakeParseResult& result);

    /**
     * @brief 分割命令参数
     * @param commandLine 命令行
     * @return 参数列表
     */
    std::vector<std::string> splitCommandArgs(const std::string& commandLine);

    /**
     * @brief 展开变量
     * @param text 包含变量的文本
     * @param result 解析结果（用于获取变量值）
     * @return 展开后的文本
     */
    std::string expandVariables(const std::string& text, const CMakeParseResult& result);

    /**
     * @brief 规范化路径
     * @param path 路径
     * @param sourceDir 源码目录
     * @return 规范化后的路径
     */
    std::string normalizePath(const std::string& path, const std::string& sourceDir);

    /**
     * @brief 检查是否为绝对路径
     * @param path 路径
     * @return 如果是绝对路径返回true
     */
    bool isAbsolutePath(const std::string& path);

    /**
     * @brief 记录调试信息
     * @param message 消息
     */
    void logDebug(const std::string& message);

    /**
     * @brief 记录警告信息
     * @param message 消息
     */
    void logWarning(const std::string& message);

    /**
     * @brief 记录错误信息
     * @param message 消息
     */
    void logError(const std::string& message);
};

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_CMAKE_PARSER_H 