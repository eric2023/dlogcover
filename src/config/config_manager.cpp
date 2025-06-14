/**
 * @file config_manager.cpp
 * @brief 配置管理器实现 - 基于compile_commands.json的简化配置管理
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config_manager.h>
#include <dlogcover/config/compile_commands_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace dlogcover {
namespace config {

ConfigManager::ConfigManager() {
    LOG_DEBUG("配置管理器初始化，加载内置默认配置");
    config_ = createDefaultConfig(""); // 使用空字符串表示非特定项目
    loaded_ = true; // 认为默认配置已加载
    compile_manager_ = std::make_unique<CompileCommandsManager>();
}

ConfigManager::~ConfigManager() {
    LOG_DEBUG("配置管理器销毁");
}

bool ConfigManager::loadConfig(const std::string& config_path) {
    LOG_INFO_FMT("加载配置文件: %s", config_path.c_str());

    if (!std::filesystem::exists(config_path)) {
        setError("配置文件不存在: " + config_path);
        return false;
    }

    if (!parseConfigFile(config_path)) {
        return false;
    }

    if (!validateConfig()) {
        return false;
    }

    loaded_ = true;
    LOG_INFO("配置文件加载成功");
    return true;
}

bool ConfigManager::initializeDefault(const std::string& project_dir) {
    LOG_INFO_FMT("初始化默认配置: 项目目录=%s", project_dir.c_str());

    config_ = createDefaultConfig(project_dir);
    
    if (!validateConfig()) {
        return false;
    }

    loaded_ = true;
    LOG_INFO("默认配置初始化成功");
    return true;
}

const Config& ConfigManager::getConfig() const {
    return config_;
}

CompileCommandsManager& ConfigManager::getCompileCommandsManager() {
    return *compile_manager_;
}

bool ConfigManager::validateConfig() const {
    LOG_DEBUG("验证配置");

    return validateProjectConfig() && 
           validateScanConfig() && 
           validateCompileCommandsConfig();
}

void ConfigManager::mergeWithCommandLineOptions(const dlogcover::cli::Options& options) {
    LOG_DEBUG("合并命令行选项");

    // 合并项目目录
    if (!options.directory.empty()) {
        config_.project.directory = options.directory;
        LOG_DEBUG_FMT("设置项目目录: %s", config_.project.directory.c_str());
    }

    // 合并排除模式
    if (!options.excludePatterns.empty()) {
        config_.scan.exclude_patterns.insert(
            config_.scan.exclude_patterns.end(),
            options.excludePatterns.begin(),
            options.excludePatterns.end()
        );
        LOG_DEBUG_FMT("添加 %zu 个命令行排除模式", options.excludePatterns.size());
    }

    // 合并输出配置
    if (!options.output_file.empty()) {
        config_.output.report_file = options.output_file;
        LOG_DEBUG_FMT("设置输出文件: %s", config_.output.report_file.c_str());
    }

    if (!options.log_file.empty()) {
        config_.output.log_file = options.log_file;
        LOG_DEBUG_FMT("设置日志文件: %s", config_.output.log_file.c_str());
    }

    // 合并日志级别
    if (options.logLevel != cli::LogLevel::UNKNOWN) {
        switch (options.logLevel) {
            case cli::LogLevel::DEBUG:   config_.output.log_level = "DEBUG";   break;
            case cli::LogLevel::INFO:    config_.output.log_level = "INFO";    break;
            case cli::LogLevel::WARNING: config_.output.log_level = "WARNING"; break;
            case cli::LogLevel::CRITICAL:config_.output.log_level = "CRITICAL";break;
            case cli::LogLevel::FATAL:   config_.output.log_level = "FATAL";   break;
            default: break; // UNKNOWN 和 ALL 在此不处理
        }
        LOG_DEBUG_FMT("设置日志级别: %s", config_.output.log_level.c_str());
    }
}

bool ConfigManager::generateDefaultConfig(const std::string& config_path, 
                                         const std::string& project_dir) {
    LOG_INFO_FMT("生成默认配置文件: %s", config_path.c_str());

    try {
        Config default_config = createDefaultConfig(project_dir);

        nlohmann::json json_config;
        
        // 项目配置
        json_config["project"]["name"] = default_config.project.name;
        json_config["project"]["directory"] = default_config.project.directory;
        json_config["project"]["build_directory"] = default_config.project.build_directory;

        // 扫描配置
        json_config["scan"]["directories"] = default_config.scan.directories;
        json_config["scan"]["file_extensions"] = default_config.scan.file_extensions;
        json_config["scan"]["exclude_patterns"] = default_config.scan.exclude_patterns;

        // 编译命令配置
        json_config["compile_commands"]["path"] = default_config.compile_commands.path;
        json_config["compile_commands"]["auto_generate"] = default_config.compile_commands.auto_generate;
        json_config["compile_commands"]["cmake_args"] = default_config.compile_commands.cmake_args;

        // 输出配置
        json_config["output"]["report_file"] = default_config.output.report_file;
        json_config["output"]["log_file"] = default_config.output.log_file;
        json_config["output"]["log_level"] = default_config.output.log_level;
        json_config["output"]["show_uncovered_paths_details"] = default_config.output.show_uncovered_paths_details;

        // 日志函数配置
        json_config["log_functions"]["qt"]["enabled"] = default_config.log_functions.qt.enabled;
        json_config["log_functions"]["qt"]["functions"] = default_config.log_functions.qt.functions;
        json_config["log_functions"]["qt"]["category_functions"] = default_config.log_functions.qt.category_functions;
        json_config["log_functions"]["custom"]["enabled"] = default_config.log_functions.custom.enabled;
        json_config["log_functions"]["custom"]["functions"] = default_config.log_functions.custom.functions;

        // 分析配置
        json_config["analysis"]["function_coverage"] = default_config.analysis.function_coverage;
        json_config["analysis"]["branch_coverage"] = default_config.analysis.branch_coverage;
        json_config["analysis"]["exception_coverage"] = default_config.analysis.exception_coverage;
        json_config["analysis"]["key_path_coverage"] = default_config.analysis.key_path_coverage;

        // 写入文件
        std::ofstream file(config_path);
        if (!file.is_open()) {
            LOG_ERROR_FMT("无法创建配置文件: %s", config_path.c_str());
            return false;
        }

        file << json_config.dump(4) << std::endl;
        LOG_INFO_FMT("成功生成默认配置文件: %s", config_path.c_str());
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR_FMT("生成默认配置文件失败: %s", e.what());
        return false;
    }
}

Config ConfigManager::createDefaultConfig(const std::string& project_dir) {
    LOG_DEBUG_FMT("创建默认配置: 项目目录=%s", project_dir.c_str());

    Config config;
    std::string actualProjectDir = project_dir.empty() ? std::filesystem::current_path().string() : project_dir;

    // 项目配置
    config.project.name = std::filesystem::path(actualProjectDir).filename().string();
    if (config.project.name.empty() || config.project.name == ".") {
        config.project.name = "dlog-project";
    }
    config.project.directory = actualProjectDir;
    config.project.build_directory = (std::filesystem::path(actualProjectDir) / "build").string();

    // 扫描配置
    config.scan.directories = {"include","src","tests"};
    config.scan.file_extensions = {".cpp", ".h", ".cxx", ".hpp"};
    config.scan.exclude_patterns = {"*build*", "*/build/*"};

    // 编译命令配置
    config.compile_commands.path = (std::filesystem::path(config.project.build_directory) / "compile_commands.json").string();
    config.compile_commands.auto_generate = true;
    config.compile_commands.cmake_args = {"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"};

    // 输出配置
    config.output.report_file = "dlogcover_report.txt";
    config.output.log_file = "dlogcover.log";
    config.output.log_level = "INFO";
    config.output.show_uncovered_paths_details = false;

    // 分析配置
    config.analysis.function_coverage = true;
    config.analysis.branch_coverage = true;
    config.analysis.exception_coverage = true;
    config.analysis.key_path_coverage = true;

    // 日志函数配置 (保持原样)
    
    return config;
}

bool ConfigManager::parseConfigFile(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            setError("无法打开配置文件: " + config_path);
            return false;
        }

        nlohmann::json json_config;
        file >> json_config;

        // 解析项目配置
        if (json_config.find("project") != json_config.end()) {
            const auto& project = json_config["project"];
            if (project.find("name") != project.end() && project["name"].is_string()) {
                config_.project.name = project["name"].get<std::string>();
            }
            if (project.find("directory") != project.end() && project["directory"].is_string()) {
                config_.project.directory = project["directory"].get<std::string>();
            }
            if (project.find("build_directory") != project.end() && project["build_directory"].is_string()) {
                config_.project.build_directory = project["build_directory"].get<std::string>();
            }
        }

        // 解析扫描配置
        if (json_config.find("scan") != json_config.end()) {
            const auto& scan = json_config["scan"];
            if (scan.find("directories") != scan.end() && scan["directories"].is_array()) {
                config_.scan.directories.clear();
                for (const auto& dir : scan["directories"]) {
                    if (dir.is_string()) {
                        config_.scan.directories.push_back(dir.get<std::string>());
                    }
                }
            }
            if (scan.find("file_extensions") != scan.end() && scan["file_extensions"].is_array()) {
                config_.scan.file_extensions.clear();
                for (const auto& ext : scan["file_extensions"]) {
                    if (ext.is_string()) {
                        config_.scan.file_extensions.push_back(ext.get<std::string>());
                    }
                }
            }
            if (scan.find("exclude_patterns") != scan.end() && scan["exclude_patterns"].is_array()) {
                config_.scan.exclude_patterns.clear();
                for (const auto& pattern : scan["exclude_patterns"]) {
                    if (pattern.is_string()) {
                        config_.scan.exclude_patterns.push_back(pattern.get<std::string>());
                    }
                }
            }
        }

        // 解析编译命令配置
        if (json_config.find("compile_commands") != json_config.end()) {
            const auto& cc = json_config["compile_commands"];
            if (cc.find("path") != cc.end() && cc["path"].is_string()) {
                config_.compile_commands.path = cc["path"].get<std::string>();
            }
            if (cc.find("auto_generate") != cc.end() && cc["auto_generate"].is_boolean()) {
                config_.compile_commands.auto_generate = cc["auto_generate"].get<bool>();
            }
            if (cc.find("cmake_args") != cc.end() && cc["cmake_args"].is_array()) {
                config_.compile_commands.cmake_args.clear();
                for (const auto& arg : cc["cmake_args"]) {
                    if (arg.is_string()) {
                        config_.compile_commands.cmake_args.push_back(arg.get<std::string>());
                    }
                }
            }
        }

        // 解析输出配置
        if (json_config.find("output") != json_config.end()) {
            const auto& output = json_config["output"];
            if (output.find("report_file") != output.end() && output["report_file"].is_string()) {
                config_.output.report_file = output["report_file"].get<std::string>();
            }
            if (output.find("log_file") != output.end() && output["log_file"].is_string()) {
                config_.output.log_file = output["log_file"].get<std::string>();
            }
            if (output.find("log_level") != output.end() && output["log_level"].is_string()) {
                config_.output.log_level = output["log_level"].get<std::string>();
            }
            if (output.find("show_uncovered_paths_details") != output.end() && output["show_uncovered_paths_details"].is_boolean()) {
                config_.output.show_uncovered_paths_details = output["show_uncovered_paths_details"].get<bool>();
            }
        }

        // 解析日志函数配置
        if (json_config.find("log_functions") != json_config.end()) {
            const auto& log_funcs = json_config["log_functions"];
            
            // 解析Qt日志函数配置
            if (log_funcs.find("qt") != log_funcs.end()) {
                const auto& qt = log_funcs["qt"];
                if (qt.find("enabled") != qt.end() && qt["enabled"].is_boolean()) {
                    config_.log_functions.qt.enabled = qt["enabled"].get<bool>();
                }
                if (qt.find("functions") != qt.end() && qt["functions"].is_array()) {
                    config_.log_functions.qt.functions.clear();
                    for (const auto& func : qt["functions"]) {
                        if (func.is_string()) {
                            config_.log_functions.qt.functions.push_back(func.get<std::string>());
                        }
                    }
                }
                if (qt.find("category_functions") != qt.end() && qt["category_functions"].is_array()) {
                    config_.log_functions.qt.category_functions.clear();
                    for (const auto& func : qt["category_functions"]) {
                        if (func.is_string()) {
                            config_.log_functions.qt.category_functions.push_back(func.get<std::string>());
                        }
                    }
                }
            }
            
            // 解析自定义日志函数配置
            if (log_funcs.find("custom") != log_funcs.end()) {
                const auto& custom = log_funcs["custom"];
                if (custom.find("enabled") != custom.end() && custom["enabled"].is_boolean()) {
                    config_.log_functions.custom.enabled = custom["enabled"].get<bool>();
                }
                if (custom.find("functions") != custom.end() && custom["functions"].is_object()) {
                    config_.log_functions.custom.functions.clear();
                    for (const auto& [level, funcs] : custom["functions"].items()) {
                        if (funcs.is_array()) {
                            std::vector<std::string> func_list;
                            for (const auto& func : funcs) {
                                if (func.is_string()) {
                                    func_list.push_back(func.get<std::string>());
                                }
                            }
                            config_.log_functions.custom.functions[level] = func_list;
                        }
                    }
                }
            }
        }

        // 解析分析配置
        if (json_config.find("analysis") != json_config.end()) {
            const auto& analysis = json_config["analysis"];
            if (analysis.find("function_coverage") != analysis.end() && analysis["function_coverage"].is_boolean()) {
                config_.analysis.function_coverage = analysis["function_coverage"].get<bool>();
            }
            if (analysis.find("branch_coverage") != analysis.end() && analysis["branch_coverage"].is_boolean()) {
                config_.analysis.branch_coverage = analysis["branch_coverage"].get<bool>();
            }
            if (analysis.find("exception_coverage") != analysis.end() && analysis["exception_coverage"].is_boolean()) {
                config_.analysis.exception_coverage = analysis["exception_coverage"].get<bool>();
            }
            if (analysis.find("key_path_coverage") != analysis.end() && analysis["key_path_coverage"].is_boolean()) {
                config_.analysis.key_path_coverage = analysis["key_path_coverage"].get<bool>();
            }
        }

        LOG_DEBUG("配置文件解析成功");
        return true;

    } catch (const std::exception& e) {
        setError("解析配置文件失败: " + std::string(e.what()));
        return false;
    }
}

void ConfigManager::setError(const std::string& message) {
    error_ = message;
    LOG_ERROR_FMT("配置管理器错误: %s", message.c_str());
}

bool ConfigManager::validateProjectConfig() const {
    if (config_.project.directory.empty()) {
        LOG_ERROR("项目目录不能为空");
        return false;
    }

    if (!std::filesystem::exists(config_.project.directory)) {
        LOG_ERROR_FMT("项目目录不存在: %s", config_.project.directory.c_str());
        return false;
    }

    return true;
}

bool ConfigManager::validateScanConfig() const {
    if (config_.scan.directories.empty()) {
        LOG_ERROR("扫描目录列表不能为空");
        return false;
    }

    if (config_.scan.file_extensions.empty()) {
        LOG_ERROR("文件扩展名列表不能为空");
        return false;
    }

    // 检查文件扩展名格式
    for (const auto& ext : config_.scan.file_extensions) {
        if (ext.empty() || ext[0] != '.') {
            LOG_ERROR_FMT("文件扩展名格式错误: %s", ext.c_str());
            return false;
        }
    }

    return true;
}

bool ConfigManager::validateCompileCommandsConfig() const {
    if (config_.compile_commands.path.empty()) {
        LOG_ERROR("compile_commands.json路径不能为空");
        return false;
    }

    return true;
}

} // namespace config
} // namespace dlogcover
