/**
 * @file compile_commands_manager.h
 * @brief 编译命令管理器 - 处理compile_commands.json的生成和解析
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CONFIG_COMPILE_COMMANDS_MANAGER_H
#define DLOGCOVER_CONFIG_COMPILE_COMMANDS_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace dlogcover {
namespace config {

/**
 * @brief 单个文件的编译信息
 */
struct CompileInfo {
    std::string directory;                   ///< 编译目录
    std::string file;                        ///< 源文件路径
    std::string command;                     ///< 完整编译命令
    std::vector<std::string> include_paths;  ///< 包含路径列表
    std::vector<std::string> defines;        ///< 宏定义列表
    std::vector<std::string> flags;          ///< 编译标志列表
};

/**
 * @brief 编译命令管理器
 * 
 * 负责生成和解析compile_commands.json文件
 */
class CompileCommandsManager {
public:
    /**
     * @brief 构造函数
     */
    CompileCommandsManager();

    /**
     * @brief 析构函数
     */
    ~CompileCommandsManager();

    /**
     * @brief 生成compile_commands.json文件
     * @param project_dir 项目根目录
     * @param build_dir 构建目录
     * @param cmake_args CMake参数
     * @return 成功返回true，失败返回false
     */
    bool generateCompileCommands(const std::string& project_dir, 
                                const std::string& build_dir,
                                const std::vector<std::string>& cmake_args = {});

    /**
     * @brief 解析compile_commands.json文件
     * @param file_path compile_commands.json文件路径
     * @return 成功返回true，失败返回false
     */
    bool parseCompileCommands(const std::string& file_path);

    /**
     * @brief 获取指定文件的编译信息
     * @param file_path 源文件路径（可以是相对路径或绝对路径）
     * @return 编译信息，如果未找到返回空的CompileInfo
     */
    CompileInfo getCompileInfoForFile(const std::string& file_path) const;

    /**
     * @brief 获取指定文件的编译器参数
     * @param file_path 源文件路径
     * @return 编译器参数列表
     */
    std::vector<std::string> getCompilerArgs(const std::string& file_path) const;

    /**
     * @brief 检查compile_commands.json是否存在且有效
     * @param file_path compile_commands.json文件路径
     * @return 存在且有效返回true，否则返回false
     */
    bool isCompileCommandsValid(const std::string& file_path) const;

    /**
     * @brief 获取所有已解析的文件列表
     * @return 文件路径列表
     */
    std::vector<std::string> getAllFiles() const;

    /**
     * @brief 获取错误信息
     * @return 错误信息字符串
     */
    const std::string& getError() const { return error_; }

    /**
     * @brief 清除已解析的数据
     */
    void clear();

    /**
     * @brief 获取项目目录
     * @param filePath 文件路径
     * @return 项目目录
     */
    std::string getProjectDirectory(const std::string& filePath) const;

private:
    std::string compile_commands_path_;
    std::map<std::string, CompileInfo> compile_info_map_;
    std::string error_;

    bool parseCompileCommands();
    std::vector<std::string> parseCompilerCommand(const std::string& command) const;
    std::string normalizePath(const std::string& path) const;
    void logError(const std::string& message) const;

    /**
     * @brief 解析单个编译命令
     * @param command 编译命令字符串
     * @param compile_info 输出的编译信息
     * @return 成功返回true，失败返回false
     */
    bool parseCompileCommand(const std::string& command, CompileInfo& compile_info);

    /**
     * @brief 检查CMake是否可用
     * @return 可用返回true，否则返回false
     */
    bool isCMakeAvailable() const;

    /**
     * @brief 运行CMake命令
     * @param project_dir 项目目录
     * @param build_dir 构建目录
     * @param cmake_args CMake参数
     * @return 成功返回true，失败返回false
     */
    bool runCMake(const std::string& project_dir, 
                  const std::string& build_dir,
                  const std::vector<std::string>& cmake_args);

    /**
     * @brief 设置错误信息
     * @param message 错误信息
     */
    void setError(const std::string& message);

    /**
     * @brief 自动检测系统头文件包含路径
     * @return 系统头文件路径列表
     */
    std::vector<std::string> detectSystemIncludes() const;
    
    /**
     * @brief 检测项目相关的包含路径
     * @param filePath 文件路径，用于推断项目根目录
     * @return 项目包含路径列表
     */
    std::vector<std::string> detectProjectIncludes(const std::string& filePath) const;
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_COMPILE_COMMANDS_MANAGER_H 