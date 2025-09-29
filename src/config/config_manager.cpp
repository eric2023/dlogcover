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

void ConfigManager::mergeWithCommandLineOptions(dlogcover::cli::Options& options) {
    // 合并include模式（命令行优先，且有include时忽略exclude）
    if (!options.includePatterns.empty()) {
        config_.scan.include_patterns = options.includePatterns;
        LOG_DEBUG_FMT("设置仅扫描模式（include_patterns），数量: %zu", options.includePatterns.size());
        LOG_DEBUG("有include参数时，自动忽略所有exclude");
    } else if (!config_.scan.include_patterns.empty()) {
        // 配置文件有include_patterns，命令行无
        options.includePatterns = config_.scan.include_patterns;
        LOG_DEBUG("配置文件有include_patterns时，自动忽略所有exclude");
    }
    LOG_DEBUG("开始合并命令行选项");

    // 合并目录配置
    if (!options.directory.empty()) {
        std::string oldProjectDir = config_.project.directory;
        config_.project.directory = options.directory;
        LOG_DEBUG_FMT("设置项目目录: %s", config_.project.directory.c_str());

        // 如果扫描目录是相对路径或默认值，需要相应调整
        if (shouldUpdateScanDirectories(oldProjectDir, options.directory)) {
            updateScanDirectoriesForNewProject(oldProjectDir, options.directory);
            LOG_INFO_FMT("根据项目目录更新扫描目录，新项目目录: %s", options.directory.c_str());
        }
    } else if (!config_.project.directory.empty()) {
        // 命令行未指定目录，但配置文件有值，回填到命令行选项
        options.directory = config_.project.directory;
        LOG_DEBUG_FMT("使用配置文件项目目录: %s", options.directory.c_str());
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

    // 合并输出配置 - 支持双向合并
    if (!options.output_file.empty()) {
        // 命令行指定了输出文件，使用命令行值
        config_.output.report_file = options.output_file;
        LOG_DEBUG_FMT("使用命令行输出文件: %s", config_.output.report_file.c_str());
    } else if (!config_.output.report_file.empty()) {
        // 命令行未指定，但配置文件有值，回填到命令行选项
        options.output_file = config_.output.report_file;
        LOG_DEBUG_FMT("使用配置文件输出文件: %s", options.output_file.c_str());
    }

    if (!options.log_file.empty()) {
        // 命令行指定了日志文件，使用命令行值
        config_.output.log_file = options.log_file;
        LOG_DEBUG_FMT("使用命令行日志文件: %s", config_.output.log_file.c_str());
    } else if (!config_.output.log_file.empty()) {
        // 命令行未指定，但配置文件有值，回填到命令行选项
        options.log_file = config_.output.log_file;
        LOG_DEBUG_FMT("使用配置文件日志文件: %s", options.log_file.c_str());
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

    // 合并性能配置
    if (options.disableParallel) {
        config_.performance.enable_parallel_analysis = false;
        LOG_DEBUG("禁用并行分析");
    }

    if (options.maxThreads > 0) {
        config_.performance.max_threads = options.maxThreads;
        LOG_DEBUG_FMT("设置最大线程数: %zu", config_.performance.max_threads);
    }

    if (options.disableCache) {
        config_.performance.enable_ast_cache = false;
        LOG_DEBUG("禁用AST缓存");
    }

    if (options.maxCacheSize > 0) {
        config_.performance.max_cache_size = options.maxCacheSize;
        LOG_DEBUG_FMT("设置最大缓存大小: %zu", config_.performance.max_cache_size);
    }

    if (options.disableIOOptimization) {
        config_.performance.enable_io_optimization = false;
        LOG_DEBUG("禁用I/O优化");
    }

    // 合并分析模式配置
    if (!options.mode.empty()) {
        // 命令行指定了分析模式，使用命令行值
        config_.analysis.mode = options.mode;
        LOG_DEBUG_FMT("设置分析模式: %s", config_.analysis.mode.c_str());
    } else if (!config_.analysis.mode.empty()) {
        // 命令行未指定，但配置文件有值，回填到命令行选项
        options.mode = config_.analysis.mode;
        LOG_DEBUG_FMT("使用配置文件分析模式: %s", options.mode.c_str());
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
        json_config["analysis"]["mode"] = default_config.analysis.mode;
        json_config["analysis"]["auto_detection"]["sample_size"] = default_config.analysis.auto_detection.sample_size;
        json_config["analysis"]["auto_detection"]["confidence_threshold"] = default_config.analysis.auto_detection.confidence_threshold;
        json_config["analysis"]["function_coverage"] = default_config.analysis.function_coverage;
        json_config["analysis"]["branch_coverage"] = default_config.analysis.branch_coverage;
        json_config["analysis"]["exception_coverage"] = default_config.analysis.exception_coverage;
        json_config["analysis"]["key_path_coverage"] = default_config.analysis.key_path_coverage;

        // 性能配置
        json_config["performance"]["enable_parallel_analysis"] = default_config.performance.enable_parallel_analysis;
        json_config["performance"]["max_threads"] = default_config.performance.max_threads;
        json_config["performance"]["enable_ast_cache"] = default_config.performance.enable_ast_cache;
        json_config["performance"]["max_cache_size"] = default_config.performance.max_cache_size;
        json_config["performance"]["enable_io_optimization"] = default_config.performance.enable_io_optimization;
        json_config["performance"]["file_buffer_size"] = default_config.performance.file_buffer_size;
        json_config["performance"]["enable_file_preloading"] = default_config.performance.enable_file_preloading;

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
    config.analysis.mode = "cpp_only";
    config.analysis.auto_detection.sample_size = 100;
    config.analysis.auto_detection.confidence_threshold = 0.9;
    config.analysis.function_coverage = true;
    config.analysis.branch_coverage = true;
    config.analysis.exception_coverage = true;
    config.analysis.key_path_coverage = true;

    // 性能配置
    config.performance.enable_parallel_analysis = true;
    config.performance.max_threads = 0; // 0表示自动检测硬件线程数
    config.performance.enable_ast_cache = true;
    config.performance.max_cache_size = 100;
    config.performance.enable_io_optimization = true;
    config.performance.file_buffer_size = 64 * 1024;
    config.performance.enable_file_preloading = true;

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
            if (scan.find("include_patterns") != scan.end() && scan["include_patterns"].is_array()) {
                config_.scan.include_patterns.clear();
                for (const auto& pattern : scan["include_patterns"]) {
                    if (pattern.is_string()) {
                        config_.scan.include_patterns.push_back(pattern.get<std::string>());
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

            // 解析分析模式
            if (analysis.find("mode") != analysis.end() && analysis["mode"].is_string()) {
                config_.analysis.mode = analysis["mode"].get<std::string>();
            }

            // 解析自动检测配置
            if (analysis.find("auto_detection") != analysis.end()) {
                const auto& auto_detect = analysis["auto_detection"];
                if (auto_detect.find("sample_size") != auto_detect.end() && auto_detect["sample_size"].is_number_unsigned()) {
                    config_.analysis.auto_detection.sample_size = auto_detect["sample_size"].get<size_t>();
                }
                if (auto_detect.find("confidence_threshold") != auto_detect.end() && auto_detect["confidence_threshold"].is_number()) {
                    config_.analysis.auto_detection.confidence_threshold = auto_detect["confidence_threshold"].get<double>();
                }
            }

            // 解析覆盖率配置
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

        // 解析性能配置
        if (json_config.find("performance") != json_config.end()) {
            const auto& performance = json_config["performance"];

            if (performance.find("enable_parallel_analysis") != performance.end() && performance["enable_parallel_analysis"].is_boolean()) {
                config_.performance.enable_parallel_analysis = performance["enable_parallel_analysis"].get<bool>();
            }
            if (performance.find("max_threads") != performance.end() && performance["max_threads"].is_number_unsigned()) {
                config_.performance.max_threads = performance["max_threads"].get<size_t>();
            }
            if (performance.find("enable_ast_cache") != performance.end() && performance["enable_ast_cache"].is_boolean()) {
                config_.performance.enable_ast_cache = performance["enable_ast_cache"].get<bool>();
            }
            if (performance.find("max_cache_size") != performance.end() && performance["max_cache_size"].is_number_unsigned()) {
                config_.performance.max_cache_size = performance["max_cache_size"].get<size_t>();
            }
            if (performance.find("enable_io_optimization") != performance.end() && performance["enable_io_optimization"].is_boolean()) {
                config_.performance.enable_io_optimization = performance["enable_io_optimization"].get<bool>();
            }
            if (performance.find("file_buffer_size") != performance.end() && performance["file_buffer_size"].is_number_unsigned()) {
                config_.performance.file_buffer_size = performance["file_buffer_size"].get<size_t>();
            }
            if (performance.find("enable_file_preloading") != performance.end() && performance["enable_file_preloading"].is_boolean()) {
                config_.performance.enable_file_preloading = performance["enable_file_preloading"].get<bool>();
            }
        }

        // 解析Go语言配置
        if (json_config.find("go") != json_config.end()) {
            const auto& go = json_config["go"];

            // 标准库log配置
            if (go.find("standard_log") != go.end()) {
                const auto& std_log = go["standard_log"];
                if (std_log.find("enabled") != std_log.end() && std_log["enabled"].is_boolean()) {
                    config_.go.standard_log.enabled = std_log["enabled"].get<bool>();
                }
                if (std_log.find("functions") != std_log.end() && std_log["functions"].is_array()) {
                    config_.go.standard_log.functions.clear();
                    for (const auto& func : std_log["functions"]) {
                        if (func.is_string()) {
                            config_.go.standard_log.functions.push_back(func.get<std::string>());
                        }
                    }
                }
            }

            // Logrus配置
            if (go.find("logrus") != go.end()) {
                const auto& logrus = go["logrus"];
                if (logrus.find("enabled") != logrus.end() && logrus["enabled"].is_boolean()) {
                    config_.go.logrus.enabled = logrus["enabled"].get<bool>();
                }
                if (logrus.find("functions") != logrus.end() && logrus["functions"].is_array()) {
                    config_.go.logrus.functions.clear();
                    for (const auto& func : logrus["functions"]) {
                        if (func.is_string()) {
                            config_.go.logrus.functions.push_back(func.get<std::string>());
                        }
                    }
                }
                if (logrus.find("formatted_functions") != logrus.end() && logrus["formatted_functions"].is_array()) {
                    config_.go.logrus.formatted_functions.clear();
                    for (const auto& func : logrus["formatted_functions"]) {
                        if (func.is_string()) {
                            config_.go.logrus.formatted_functions.push_back(func.get<std::string>());
                        }
                    }
                }
                if (logrus.find("line_functions") != logrus.end() && logrus["line_functions"].is_array()) {
                    config_.go.logrus.line_functions.clear();
                    for (const auto& func : logrus["line_functions"]) {
                        if (func.is_string()) {
                            config_.go.logrus.line_functions.push_back(func.get<std::string>());
                        }
                    }
                }
            }

            // Zap配置
            if (go.find("zap") != go.end()) {
                const auto& zap = go["zap"];
                if (zap.find("enabled") != zap.end() && zap["enabled"].is_boolean()) {
                    config_.go.zap.enabled = zap["enabled"].get<bool>();
                }
                if (zap.find("logger_functions") != zap.end() && zap["logger_functions"].is_array()) {
                    config_.go.zap.logger_functions.clear();
                    for (const auto& func : zap["logger_functions"]) {
                        if (func.is_string()) {
                            config_.go.zap.logger_functions.push_back(func.get<std::string>());
                        }
                    }
                }
                if (zap.find("sugared_functions") != zap.end() && zap["sugared_functions"].is_array()) {
                    config_.go.zap.sugared_functions.clear();
                    for (const auto& func : zap["sugared_functions"]) {
                        if (func.is_string()) {
                            config_.go.zap.sugared_functions.push_back(func.get<std::string>());
                        }
                    }
                }
            }

            // golib配置
            if (go.find("golib") != go.end()) {
                const auto& golib = go["golib"];
                if (golib.find("enabled") != golib.end() && golib["enabled"].is_boolean()) {
                    config_.go.golib.enabled = golib["enabled"].get<bool>();
                }
                if (golib.find("functions") != golib.end() && golib["functions"].is_array()) {
                    config_.go.golib.functions.clear();
                    for (const auto& func : golib["functions"]) {
                        if (func.is_string()) {
                            config_.go.golib.functions.push_back(func.get<std::string>());
                        }
                    }
                }
                if (golib.find("formatted_functions") != golib.end() && golib["formatted_functions"].is_array()) {
                    config_.go.golib.formatted_functions.clear();
                    for (const auto& func : golib["formatted_functions"]) {
                        if (func.is_string()) {
                            config_.go.golib.formatted_functions.push_back(func.get<std::string>());
                        }
                    }
                }
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

bool ConfigManager::shouldUpdateScanDirectories(const std::string& oldProjectDir, const std::string& newProjectDir) const {
    // 如果新旧项目目录相同，不需要更新
    if (oldProjectDir == newProjectDir) {
        return false;
    }

    // 检查扫描目录是否包含相对路径或特殊情况
    for (const auto& dir : config_.scan.directories) {
        // 如果是当前目录标记
        if (dir == ".") {
            return true;
        }

        // 如果是相对路径（不以/开头）
        if (!dir.empty() && dir[0] != '/') {
            return true;
        }

        // 如果是默认的C++项目目录
        if (dir == "include" || dir == "src" || dir == "tests") {
            return true;
        }
    }

    return false;
}

void ConfigManager::updateScanDirectoriesForNewProject(const std::string& oldProjectDir, const std::string& newProjectDir) {
    std::vector<std::string> updatedDirectories;

    LOG_DEBUG_FMT("更新扫描目录：从 %s 到 %s", oldProjectDir.c_str(), newProjectDir.c_str());

    for (const auto& dir : config_.scan.directories) {
        std::string updatedDir;

        if (dir == ".") {
            // 当前目录标记改为新的项目目录
            updatedDir = newProjectDir;
            LOG_DEBUG_FMT("扫描目录 '.' 更新为: %s", updatedDir.c_str());
        } else if (!dir.empty() && dir[0] != '/') {
            // 相对路径，转换为相对于新项目目录的路径
            std::filesystem::path newPath = std::filesystem::path(newProjectDir) / dir;
            updatedDir = newPath.string();
            LOG_DEBUG_FMT("相对路径 '%s' 更新为: %s", dir.c_str(), updatedDir.c_str());
        } else {
            // 绝对路径保持不变
            updatedDir = dir;
            LOG_DEBUG_FMT("绝对路径 '%s' 保持不变", dir.c_str());
        }

        updatedDirectories.push_back(updatedDir);
    }

    config_.scan.directories = updatedDirectories;

    // 输出更新后的扫描目录列表
    LOG_INFO("扫描目录已更新:");
    for (const auto& dir : config_.scan.directories) {
        LOG_INFO_FMT("  - %s", dir.c_str());
    }
}

} // namespace config
} // namespace dlogcover
