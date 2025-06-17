/**
 * @file go_analyzer.cpp
 * @brief Go语言分析器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/analyzer/go_analyzer.h>
#include <dlogcover/core/language_detector/language_detector.h>
#include <dlogcover/utils/log_utils.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <sstream>
#include <ctime>
#include <thread>
#include <algorithm>
#include <iomanip>

namespace dlogcover {
namespace core {
namespace analyzer {

GoAnalyzer::GoAnalyzer(const config::Config& config)
    : config_(config) {
    LOG_DEBUG("初始化Go分析器");
    
    // 查找Go分析器工具
    goAnalyzerPath_ = findGoAnalyzerTool();
    
    LOG_INFO("Go语言支持已启用");
    if (goAnalyzerPath_.empty()) {
        LOG_WARNING("未找到Go分析器工具，Go语言分析功能将受限");
    } else {
        LOG_INFO_FMT("Go分析器工具路径: %s", goAnalyzerPath_.c_str());
    }
    
    // 强制输出路径查找结果用于调试
    LOG_INFO_FMT("Go分析器路径查找结果: '%s'", goAnalyzerPath_.c_str());
}

GoAnalyzer::~GoAnalyzer() {
    LOG_DEBUG("销毁Go分析器");
}

ast_analyzer::Result<bool> GoAnalyzer::analyze(const std::string& filePath) {
    LOG_DEBUG_FMT("Go分析器分析文件: %s", filePath.c_str());
   
    // 检查文件是否为Go文件
    auto language = language_detector::LanguageDetector::detectLanguage(filePath);
    if (language != language_detector::SourceLanguage::GO) {
        LOG_WARNING_FMT("文件不是Go文件，跳过分析: %s", filePath.c_str());
        return ast_analyzer::makeSuccess(true);
    }
    
    // 检查文件是否存在
    if (!std::filesystem::exists(filePath)) {
        LOG_ERROR_FMT("Go文件不存在: %s", filePath.c_str());
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::FILE_NOT_FOUND,
            "Go文件不存在: " + filePath
        );
    }
    
    // 缓存检查：如果启用缓存且缓存中有有效结果，直接使用缓存
    if (cacheEnabled_ && isCacheValid(filePath)) {
        LOG_DEBUG_FMT("使用缓存结果: %s", filePath.c_str());
        cacheHits_++;
        
        auto cachedResults = getCacheResult(filePath);
        // 将缓存结果复制到当前结果集合中
        for (const auto& result : cachedResults) {
            if (result) {
                auto resultCopy = std::make_unique<ast_analyzer::ASTNodeInfo>(*result);
                results_.push_back(std::move(resultCopy));
            }
        }
        
        // 更新统计信息
        statistics_.analyzedFiles++;
        statistics_.totalFunctions += cachedResults.size();
        for (const auto& node : cachedResults) {
            if (node && node->hasLogging) {
                statistics_.totalLogCalls += node->children.size();
            }
        }
        
        LOG_INFO_FMT("Go文件缓存分析完成: %s，找到 %zu 个函数", filePath.c_str(), cachedResults.size());
        return ast_analyzer::makeSuccess(true);
    }
    
    // 缓存未命中，执行实际分析
    if (cacheEnabled_) {
        cacheMisses_++;
    }
    
    // 阶段2完整实现：执行Go分析器
    LOG_INFO_FMT("开始分析Go文件: %s", filePath.c_str());
    
    // 执行Go分析器
    auto result = executeGoAnalyzer(filePath);
    if (result.hasError()) {
        LOG_ERROR_FMT("Go分析器执行失败: %s, 错误: %s", filePath.c_str(), result.errorMessage().c_str());
        return ast_analyzer::makeError<bool>(ast_analyzer::ASTAnalyzerError::ANALYSIS_ERROR, result.errorMessage());
    }
    
    // 解析JSON结果
    auto parseResult = parseGoAnalysisResult(result.value());
    if (parseResult.hasError()) {
        LOG_ERROR_FMT("解析Go分析结果失败: %s, 错误: %s", filePath.c_str(), parseResult.errorMessage().c_str());
        return ast_analyzer::makeError<bool>(ast_analyzer::ASTAnalyzerError::PARSE_ERROR, parseResult.errorMessage());
    }
    
    // 添加分析结果
    auto astNodes = std::move(parseResult.value());
    
    // 如果启用缓存，将结果添加到缓存
    if (cacheEnabled_) {
        addToCache(filePath, astNodes);
    }
    
    // 更新统计信息（在移动节点之前）
    statistics_.analyzedFiles++;
    statistics_.totalFunctions += astNodes.size();
    LOG_DEBUG_FMT("开始更新Go分析器统计，处理 %zu 个节点", astNodes.size());
    for (const auto& node : astNodes) {
        if (node) {
            LOG_DEBUG_FMT("检查节点: %s, hasLogging=%s, children=%zu", 
                        node->name.c_str(), 
                        node->hasLogging ? "true" : "false",
                        node->children.size());
            if (node->hasLogging) {
                // 计算这个函数中的日志调用数量（子节点）
                size_t logCalls = node->children.size();
                LOG_DEBUG_FMT("函数 %s 有 %zu 个日志调用子节点", node->name.c_str(), logCalls);
                statistics_.totalLogCalls += logCalls;
            }
        } else {
            LOG_DEBUG("发现空节点");
        }
    }
    
    // 将节点移动到结果集合中
    for (auto& node : astNodes) {
        results_.push_back(std::move(node));
    }
    LOG_DEBUG_FMT("Go分析器统计更新完成：文件=%d, 函数=%d, 日志调用=%d", 
                statistics_.analyzedFiles, statistics_.totalFunctions, statistics_.totalLogCalls);
    
    LOG_INFO_FMT("Go文件分析完成: %s，找到 %zu 个函数", filePath.c_str(), astNodes.size());
    return ast_analyzer::makeSuccess(true);
}

const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& GoAnalyzer::getResults() const {
    return results_;
}

void GoAnalyzer::clear() {
    LOG_DEBUG("清空Go分析器结果");
    results_.clear();
    statistics_ = Statistics{};
}

std::string GoAnalyzer::getLanguageName() const {
    return "Go";
}

bool GoAnalyzer::isEnabled() const {
    return goAnalyzerPath_.empty();
}

std::vector<std::string> GoAnalyzer::getSupportedExtensions() const {
    return {".go"};
}

std::string GoAnalyzer::getStatistics() const {
    return "Go分析统计: " + std::to_string(statistics_.analyzedFiles) + " 个文件, " +
           std::to_string(statistics_.totalFunctions) + " 个函数, " +
           std::to_string(statistics_.totalLogCalls) + " 个日志调用";
}

std::string GoAnalyzer::findGoAnalyzerTool() {
    std::vector<std::string> searchPaths;
    
    // 获取当前程序执行文件所在目录
    std::string executableDir;
    try {
        auto executablePath = std::filesystem::canonical("/proc/self/exe");
        executableDir = executablePath.parent_path().string();
    } catch (const std::exception& e) {
        LOG_DEBUG_FMT("无法获取程序路径: %s", e.what());
        executableDir = ".";  // 回退到当前目录
    }
    
    // 基于程序目录的相对路径
    searchPaths.push_back(executableDir + "/dlogcover-go-analyzer");  // 同目录
    
    // 基于当前工作目录的相对路径
    searchPaths.push_back("../build/bin/dlogcover-go-analyzer");      // 相对于test_project等子目录
    searchPaths.push_back("./build/bin/dlogcover-go-analyzer");       // 相对于项目根目录
    searchPaths.push_back("./tools/go-analyzer/dlogcover-go-analyzer");
    searchPaths.push_back("./dlogcover-go-analyzer");
    searchPaths.push_back("dlogcover-go-analyzer");                   // PATH中查找
    
    for (const auto& path : searchPaths) {
        LOG_DEBUG_FMT("检查Go分析器路径: %s", path.c_str());
        if (std::filesystem::exists(path)) {
            LOG_DEBUG_FMT("找到Go分析器工具: %s", path.c_str());
            return path;
        } else {
            LOG_DEBUG_FMT("路径不存在: %s", path.c_str());
        }
    }
    
    // 如果没有找到编译好的二进制文件，尝试使用go run
    std::string goSourcePath = "./tools/go-analyzer/main.go";
    if (std::filesystem::exists(goSourcePath)) {
        LOG_DEBUG_FMT("找到Go源文件，将使用go run: %s", goSourcePath.c_str());
        return "go run " + goSourcePath;
    }
    
    LOG_WARNING("未找到Go分析器工具。请确保：");
    LOG_WARNING("1. Go环境已正确安装 (go version >= 1.19)");
    LOG_WARNING("2. Go分析器源文件存在: ./tools/go-analyzer/main.go");
    LOG_WARNING("3. 或者已编译的Go分析器二进制文件存在");
    LOG_WARNING("请参考README.md中的环境搭建说明");
    return "";  // 返回空字符串表示未找到
}

ast_analyzer::Result<std::string> GoAnalyzer::executeGoAnalyzer(const std::string& filePath) {
    LOG_DEBUG_FMT("执行Go分析器: %s", filePath.c_str());
    
    // 生成配置文件
    std::string configPath = generateGoConfig();
    if (configPath.empty()) {
        return ast_analyzer::makeError<std::string>(
            ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
            "生成Go分析器配置失败"
        );
    }
    
    // 创建请求JSON
    std::string requestPath = "/tmp/dlogcover_go_request.json";
    std::ofstream requestFile(requestPath);
    if (!requestFile.is_open()) {
        return ast_analyzer::makeError<std::string>(
            ast_analyzer::ASTAnalyzerError::FILE_READ_ERROR,
            "无法创建请求文件: " + requestPath
        );
    }
    
    // 构建请求JSON
    requestFile << "{\n";
    requestFile << "  \"file_path\": \"" << filePath << "\",\n";
    requestFile << "  \"config\": {\n";
    requestFile << "    \"standard_log\": {\n";
    requestFile << "      \"enabled\": " << (config_.go.standard_log.enabled ? "true" : "false") << ",\n";
    requestFile << "      \"functions\": [";
    for (size_t i = 0; i < config_.go.standard_log.functions.size(); ++i) {
        if (i > 0) requestFile << ", ";
        requestFile << "\"" << config_.go.standard_log.functions[i] << "\"";
    }
    requestFile << "]\n";
    requestFile << "    },\n";
    requestFile << "    \"logrus\": {\n";
    requestFile << "      \"enabled\": " << (config_.go.logrus.enabled ? "true" : "false") << ",\n";
    requestFile << "      \"functions\": [";
    for (size_t i = 0; i < config_.go.logrus.functions.size(); ++i) {
        if (i > 0) requestFile << ", ";
        requestFile << "\"" << config_.go.logrus.functions[i] << "\"";
    }
    requestFile << "]\n";
    requestFile << "    },\n";
    requestFile << "    \"zap\": {\n";
    requestFile << "      \"enabled\": " << (config_.go.zap.enabled ? "true" : "false") << ",\n";
    requestFile << "      \"logger_functions\": [";
    for (size_t i = 0; i < config_.go.zap.logger_functions.size(); ++i) {
        if (i > 0) requestFile << ", ";
        requestFile << "\"" << config_.go.zap.logger_functions[i] << "\"";
    }
    requestFile << "],\n";
    requestFile << "      \"sugared_functions\": [";
    for (size_t i = 0; i < config_.go.zap.sugared_functions.size(); ++i) {
        if (i > 0) requestFile << ", ";
        requestFile << "\"" << config_.go.zap.sugared_functions[i] << "\"";
    }
    requestFile << "]\n";
    requestFile << "    },\n";
    requestFile << "    \"golib\": {\n";
    requestFile << "      \"enabled\": " << (config_.go.golib.enabled ? "true" : "false") << ",\n";
    requestFile << "      \"functions\": [";
    for (size_t i = 0; i < config_.go.golib.functions.size(); ++i) {
        if (i > 0) requestFile << ", ";
        requestFile << "\"" << config_.go.golib.functions[i] << "\"";
    }
    requestFile << "]\n";
    requestFile << "    }\n";
    requestFile << "  }\n";
    requestFile << "}\n";
    requestFile.close();
    
    // 检查Go分析器路径
    if (goAnalyzerPath_.empty()) {
        return ast_analyzer::makeError<std::string>(
            ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
            "Go分析器工具未找到，请检查环境配置"
        );
    }
    
    // 构建命令
    std::string command = goAnalyzerPath_ + " " + requestPath;
    
    // DEBUG: 输出请求内容
    LOG_DEBUG("Go分析器请求文件内容:");
    std::ifstream requestDebug(requestPath);
    std::string line;
    while (std::getline(requestDebug, line)) {
        LOG_DEBUG_FMT("  %s", line.c_str());
    }
    requestDebug.close();
    
    // 执行命令
    auto result = executeCommand(command);
    
    // DEBUG: 输出结果
    if (result.isSuccess()) {
        LOG_DEBUG_FMT("Go分析器输出结果: %s", result.value().c_str());
    } else {
        LOG_DEBUG_FMT("Go分析器执行失败: %s", result.errorMessage().c_str());
    }
    
    // 清理临时文件
    std::filesystem::remove(requestPath);
    std::filesystem::remove(configPath);
    
    return result;
}

std::string GoAnalyzer::generateGoConfig() {
    LOG_DEBUG("生成Go分析器配置");
    
    std::string configPath = "/tmp/dlogcover_go_config.json";
    std::ofstream configFile(configPath);
    if (!configFile.is_open()) {
        LOG_ERROR_FMT("无法创建Go配置文件: %s", configPath.c_str());
        return "";
    }
    
    // 生成配置JSON
    nlohmann::json config;
    
    // 标准库log配置
    config["standard_log"]["enabled"] = config_.go.standard_log.enabled;
    config["standard_log"]["functions"] = config_.go.standard_log.functions;
    
    // Logrus配置
    config["logrus"]["enabled"] = config_.go.logrus.enabled;
    config["logrus"]["functions"] = config_.go.logrus.functions;
    
    // Zap配置
    config["zap"]["enabled"] = config_.go.zap.enabled;
    config["zap"]["logger_functions"] = config_.go.zap.logger_functions;
    config["zap"]["sugared_functions"] = config_.go.zap.sugared_functions;
    
    // Golib配置
    config["golib"]["enabled"] = config_.go.golib.enabled;
    config["golib"]["functions"] = config_.go.golib.functions;
    
    configFile << config.dump(2);
    configFile.close();
    
    LOG_DEBUG_FMT("Go配置文件生成完成: %s", configPath.c_str());
    return configPath;
}

ast_analyzer::Result<std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>> 
GoAnalyzer::parseGoAnalysisResult(const std::string& jsonResult) {
    LOG_DEBUG("解析Go分析器结果");
    
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> results;
    
    try {
        // 解析JSON
        auto json = nlohmann::json::parse(jsonResult);
        
        // 检查是否成功
        if (json.find("success") == json.end() || !json["success"].get<bool>()) {
            std::string error = json.find("error") != json.end() ? json["error"].get<std::string>() : "未知错误";
            return ast_analyzer::makeError<std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>>(
                ast_analyzer::ASTAnalyzerError::ANALYSIS_ERROR,
                "Go分析器执行失败: " + error
            );
        }
        
        // 解析函数信息
        if (json.find("functions") != json.end()) {
            for (const auto& funcJson : json["functions"]) {
                auto nodeInfo = std::make_unique<ast_analyzer::ASTNodeInfo>();
                
                // 基本信息
                nodeInfo->type = ast_analyzer::NodeType::FUNCTION;
                nodeInfo->name = funcJson.value("name", "unknown");
                nodeInfo->location.line = funcJson.value("line", 0);
                nodeInfo->location.column = funcJson.value("column", 0);
                nodeInfo->endLocation.line = funcJson.value("end_line", 0);
                nodeInfo->endLocation.column = funcJson.value("end_column", 0);
                nodeInfo->hasLogging = funcJson.value("has_logging", false);
                
                LOG_DEBUG_FMT("解析Go函数: %s, has_logging=%s", 
                            nodeInfo->name.c_str(), 
                            nodeInfo->hasLogging ? "true" : "false");
                
                // 设置文件路径
                if (json.find("file_path") != json.end()) {
                    nodeInfo->location.filePath = json["file_path"].get<std::string>();
                    nodeInfo->location.fileName = std::filesystem::path(nodeInfo->location.filePath).filename().string();
                }
                
                // 解析日志调用（作为子节点）
                if (funcJson.find("log_calls") != funcJson.end()) {
                    size_t logCallCount = funcJson["log_calls"].size();
                    LOG_DEBUG_FMT("函数 %s 有 %zu 个日志调用", nodeInfo->name.c_str(), logCallCount);
                    
                    for (const auto& logCallJson : funcJson["log_calls"]) {
                        auto logNode = std::make_unique<ast_analyzer::ASTNodeInfo>();
                        logNode->type = ast_analyzer::NodeType::LOG_CALL;
                        logNode->name = logCallJson.value("function_name", "unknown");
                        logNode->location.line = logCallJson.value("line", 0);
                        logNode->location.column = logCallJson.value("column", 0);
                        logNode->location.filePath = nodeInfo->location.filePath;
                        logNode->location.fileName = nodeInfo->location.fileName;
                        logNode->hasLogging = true;
                        
                        // 构建日志调用的文本描述
                        std::string library = logCallJson.value("library", "unknown");
                        std::string level = logCallJson.value("level", "info");
                        logNode->text = library + " " + level + " log call";
                        
                        LOG_DEBUG_FMT("添加日志调用子节点: %s (line %d)", 
                                    logNode->name.c_str(), logNode->location.line);
                        
                        nodeInfo->children.push_back(std::move(logNode));
                    }
                    
                    LOG_DEBUG_FMT("函数 %s 最终有 %zu 个子节点", 
                                nodeInfo->name.c_str(), nodeInfo->children.size());
                }
                
                results.push_back(std::move(nodeInfo));
            }
        }
        
        LOG_DEBUG_FMT("成功解析Go分析结果，找到 %zu 个函数", results.size());
        
    } catch (const nlohmann::json::exception& e) {
        return ast_analyzer::makeError<std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>>(
            ast_analyzer::ASTAnalyzerError::PARSE_ERROR,
            "JSON解析失败: " + std::string(e.what())
        );
    }
    
    return ast_analyzer::makeSuccess(std::move(results));
}

ast_analyzer::Result<std::string> GoAnalyzer::executeCommand(const std::string& command) {
    LOG_DEBUG_FMT("执行命令: %s", command.c_str());
    
    // 使用popen执行命令
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return ast_analyzer::makeError<std::string>(
            ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
            "无法执行命令: " + command
        );
    }
    
    std::stringstream result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }
    
    int exitCode = pclose(pipe);
    if (exitCode != 0) {
        return ast_analyzer::makeError<std::string>(
            ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR,
            "命令执行失败，退出码: " + std::to_string(exitCode)
        );
    }
    
    return ast_analyzer::makeSuccess(result.str());
}

void GoAnalyzer::setParallelMode(bool enabled, size_t maxThreads) {
    parallelEnabled_ = enabled;
    maxThreads_ = maxThreads;
    
    LOG_INFO_FMT("Go分析器并行模式设置: %s, 最大线程数: %zu", 
                enabled ? "启用" : "禁用", maxThreads);
}

ast_analyzer::Result<bool> GoAnalyzer::analyzeFiles(const std::vector<std::string>& filePaths) {
    LOG_INFO_FMT("Go分析器批量分析 %zu 个文件", filePaths.size());
    
    if (filePaths.empty()) {
        LOG_WARNING("没有文件需要分析");
        return ast_analyzer::makeSuccess(true);
    }
    
    // 如果启用并行模式，使用并行分析
    if (parallelEnabled_) {
        return analyzeFilesParallel(filePaths);
    } else {
        return analyzeFilesSerial(filePaths);
    }
}

ast_analyzer::Result<bool> GoAnalyzer::analyzeFilesSerial(const std::vector<std::string>& filePaths) {
    LOG_DEBUG_FMT("Go分析器串行分析 %zu 个文件", filePaths.size());
    
    bool hasErrors = false;
    for (const auto& filePath : filePaths) {
        auto result = analyze(filePath);
        if (result.hasError()) {
            LOG_ERROR_FMT("Go文件分析失败: %s, 错误: %s", 
                         filePath.c_str(), result.errorMessage().c_str());
            hasErrors = true;
        }
    }
    
    if (hasErrors) {
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::ANALYSIS_ERROR,
            "部分Go文件分析失败"
        );
    }
    
    return ast_analyzer::makeSuccess(true);
}

ast_analyzer::Result<bool> GoAnalyzer::analyzeFilesParallel(const std::vector<std::string>& filePaths) {
    LOG_INFO_FMT("Go分析器开始并行分析 %zu 个文件", filePaths.size());
    
    // 确定线程数 - 参照AST分析器的逻辑
    size_t numThreads = maxThreads_;
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4; // 默认4个线程
        }
    }
    
    // 限制线程数不超过文件数
    numThreads = std::min(numThreads, filePaths.size());
    
    LOG_INFO_FMT("使用 %zu 个线程并行分析 %zu 个Go文件", numThreads, filePaths.size());
    
    // 如果只有一个文件或一个线程，回退到串行处理
    if (numThreads <= 1 || filePaths.size() <= 1) {
        LOG_INFO("文件数量较少，使用串行处理");
        return analyzeFilesSerial(filePaths);
    }
    
    // 生成批量分析配置
    std::string batchConfigFile = generateBatchAnalysisConfig(filePaths, numThreads);
    if (batchConfigFile.empty()) {
        LOG_ERROR("生成批量分析配置失败，回退到串行分析");
        return analyzeFilesSerial(filePaths);
    }
    
    // 调用Go工具的并行分析功能，使用计算出的线程数
    std::string cmd = goAnalyzerPath_ + 
                     " --mode=batch" +
                     " --config=" + batchConfigFile +
                     " --parallel=" + std::to_string(numThreads) +
                     " --output=json";
    
    LOG_DEBUG_FMT("执行Go批量分析命令: %s", cmd.c_str());
    
    auto result = executeCommand(cmd);
    
    // 清理临时配置文件
    std::filesystem::remove(batchConfigFile);
    
    if (result.hasError()) {
        LOG_ERROR_FMT("Go批量分析失败: %s，回退到串行分析", result.errorMessage().c_str());
        return analyzeFilesSerial(filePaths);
    }
    
    LOG_INFO_FMT("Go并行分析完成，使用了 %zu 个线程", numThreads);
    
    // 解析批量分析结果
    return parseBatchAnalysisResult(result.value());
}

std::string GoAnalyzer::generateBatchAnalysisConfig(const std::vector<std::string>& filePaths, size_t numThreads) {
    LOG_DEBUG_FMT("生成Go批量分析配置，使用 %zu 个线程", numThreads);
    
    std::string configPath = "/tmp/dlogcover_go_batch_" + 
                            std::to_string(std::time(nullptr)) + ".json";
    std::ofstream configFile(configPath);
    if (!configFile.is_open()) {
        LOG_ERROR_FMT("无法创建Go批量配置文件: %s", configPath.c_str());
        return "";
    }
    
    // 生成批量配置JSON
    nlohmann::json batchConfig;
    batchConfig["files"] = filePaths;
    batchConfig["parallel"] = numThreads;
    
    // 添加Go分析配置
    nlohmann::json goConfig;
    goConfig["standard_log"]["enabled"] = config_.go.standard_log.enabled;
    goConfig["standard_log"]["functions"] = config_.go.standard_log.functions;
    goConfig["logrus"]["enabled"] = config_.go.logrus.enabled;
    goConfig["logrus"]["functions"] = config_.go.logrus.functions;
    goConfig["zap"]["enabled"] = config_.go.zap.enabled;
    goConfig["zap"]["logger_functions"] = config_.go.zap.logger_functions;
    goConfig["zap"]["sugared_functions"] = config_.go.zap.sugared_functions;
    goConfig["golib"]["enabled"] = config_.go.golib.enabled;
    goConfig["golib"]["functions"] = config_.go.golib.functions;
    
    batchConfig["config"] = goConfig;
    
    configFile << batchConfig.dump(2);
    configFile.close();
    
    LOG_DEBUG_FMT("Go批量配置文件生成完成: %s", configPath.c_str());
    return configPath;
}

ast_analyzer::Result<bool> GoAnalyzer::parseBatchAnalysisResult(const std::string& jsonResult) {
    LOG_DEBUG("解析Go批量分析结果");
    
    try {
        auto json = nlohmann::json::parse(jsonResult);
        
        // 检查批量分析是否成功
        if (json.find("success") == json.end() || !json["success"].get<bool>()) {
            std::string error = json.find("error") != json.end() ? json["error"].get<std::string>() : "未知错误";
            return ast_analyzer::makeError<bool>(
                ast_analyzer::ASTAnalyzerError::ANALYSIS_ERROR,
                "Go批量分析失败: " + error
            );
        }
        
        // 解析每个文件的结果
        if (json.find("results") != json.end()) {
            for (const auto& fileResult : json["results"]) {
                if (fileResult.find("success") != fileResult.end() && fileResult["success"].get<bool>()) {
                    // 解析单个文件的分析结果
                    auto parseResult = parseGoAnalysisResult(fileResult.dump());
                    if (parseResult.hasError()) {
                        LOG_WARNING_FMT("解析文件结果失败: %s", parseResult.errorMessage().c_str());
                        continue;
                    }
                    
                    // 将结果添加到总结果中
                    auto fileResults = std::move(parseResult.value());
                    for (auto& result : fileResults) {
                        results_.push_back(std::move(result));
                    }
                }
            }
        }
        
        // 更新统计信息
        if (json.find("statistics") != json.end()) {
            const auto& stats = json["statistics"];
            statistics_.analyzedFiles += stats.value("processed_files", 0);
            statistics_.totalFunctions += stats.value("total_functions", 0);
            statistics_.totalLogCalls += stats.value("total_log_calls", 0);
        }
        
        LOG_INFO_FMT("Go批量分析完成，处理了 %zu 个结果", results_.size());
        
    } catch (const nlohmann::json::exception& e) {
        return ast_analyzer::makeError<bool>(
            ast_analyzer::ASTAnalyzerError::PARSE_ERROR,
            "批量分析结果JSON解析失败: " + std::string(e.what())
        );
    }
    
    return ast_analyzer::makeSuccess(true);
}

// ===== 缓存功能实现 =====

void GoAnalyzer::enableCache(bool enabled, size_t maxCacheSize, size_t maxMemoryMB) {
    LOG_DEBUG_FMT("设置Go分析器缓存: enabled=%s, maxCacheSize=%zu, maxMemoryMB=%zu", 
                  enabled ? "true" : "false", maxCacheSize, maxMemoryMB);
    
    cacheEnabled_ = enabled;
    maxCacheSize_ = maxCacheSize;
    maxMemoryMB_ = maxMemoryMB;
    
    if (!enabled) {
        clearCache();
    }
}

std::string GoAnalyzer::getCacheStatistics() const {
    if (!cacheEnabled_) {
        return "Go分析器缓存未启用";
    }
    
    double hitRate = (cacheHits_ + cacheMisses_ > 0) ? 
                     (static_cast<double>(cacheHits_) / (cacheHits_ + cacheMisses_) * 100.0) : 0.0;
    
    std::stringstream ss;
    ss << "Go分析器缓存统计信息:\n";
    ss << "  缓存条目数: " << cache_.size() << "/" << maxCacheSize_ << "\n";
    ss << "  缓存命中: " << cacheHits_ << "\n";
    ss << "  缓存未命中: " << cacheMisses_ << "\n";
    ss << "  总访问次数: " << (cacheHits_ + cacheMisses_) << "\n";
    ss << "  命中率: " << std::fixed << std::setprecision(2) << hitRate << "%\n";
    ss << "  内存使用: " << std::fixed << std::setprecision(2) 
       << (currentMemoryUsage_ / 1024.0 / 1024.0) << " MB / " << maxMemoryMB_ << " MB";
    
    return ss.str();
}

void GoAnalyzer::clearCache() {
    LOG_DEBUG("清空Go分析器缓存");
    cache_.clear();
    cacheHits_ = 0;
    cacheMisses_ = 0;
    currentMemoryUsage_ = 0;
}

std::string GoAnalyzer::calculateFileHash(const std::string& filePath) const {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    // 简单的哈希计算（实际项目中可以使用MD5或SHA256）
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::hash<std::string> hasher;
    size_t hashValue = hasher(content);
    
    std::stringstream ss;
    ss << std::hex << hashValue;
    return ss.str();
}

std::time_t GoAnalyzer::getFileModifiedTime(const std::string& filePath) const {
    try {
        auto ftime = std::filesystem::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return std::chrono::system_clock::to_time_t(sctp);
    } catch (const std::exception& e) {
        LOG_DEBUG_FMT("获取文件修改时间失败: %s, 错误: %s", filePath.c_str(), e.what());
        return 0;
    }
}

bool GoAnalyzer::isCacheValid(const std::string& filePath) const {
    auto it = cache_.find(filePath);
    if (it == cache_.end()) {
        return false;
    }
    
    // 检查文件修改时间
    std::time_t currentModTime = getFileModifiedTime(filePath);
    if (currentModTime != it->second.lastModified) {
        LOG_DEBUG_FMT("文件已修改，缓存失效: %s", filePath.c_str());
        return false;
    }
    
    // 检查文件哈希值（更严格的验证）
    std::string currentHash = calculateFileHash(filePath);
    if (currentHash != it->second.fileHash) {
        LOG_DEBUG_FMT("文件内容已变更，缓存失效: %s", filePath.c_str());
        return false;
    }
    
    // 更新访问时间（用于LRU）
    it->second.accessTime = std::time(nullptr);
    
    return true;
}

std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> GoAnalyzer::getCacheResult(const std::string& filePath) const {
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> results;
    
    auto it = cache_.find(filePath);
    if (it != cache_.end()) {
        // 创建缓存结果的深拷贝
        for (const auto& result : it->second.results) {
            if (result) {
                auto copy = std::make_unique<ast_analyzer::ASTNodeInfo>(*result);
                results.push_back(std::move(copy));
            }
        }
        
        // 更新访问时间
        it->second.accessTime = std::time(nullptr);
    }
    
    return results;
}

void GoAnalyzer::addToCache(const std::string& filePath, 
                           const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& results) const {
    
    // 检查缓存是否已满，需要清理
    if (cache_.size() >= maxCacheSize_) {
        evictLRUCache();
    }
    
    // 创建缓存条目
    CacheEntry entry;
    entry.fileHash = calculateFileHash(filePath);
    entry.lastModified = getFileModifiedTime(filePath);
    entry.accessTime = std::time(nullptr);
    
    // 创建结果的深拷贝
    for (const auto& result : results) {
        if (result) {
            auto copy = std::make_unique<ast_analyzer::ASTNodeInfo>(*result);
            entry.results.push_back(std::move(copy));
        }
    }
    
    // 估算内存使用
    entry.memorySize = estimateMemoryUsage(results);
    
    // 检查内存限制
    if (currentMemoryUsage_ + entry.memorySize > maxMemoryMB_ * 1024 * 1024) {
        // 内存不足，清理缓存
        while (!cache_.empty() && currentMemoryUsage_ + entry.memorySize > maxMemoryMB_ * 1024 * 1024) {
            evictLRUCache();
        }
    }
    
    // 如果条目已存在，先移除旧的
    auto it = cache_.find(filePath);
    if (it != cache_.end()) {
        currentMemoryUsage_ -= it->second.memorySize;
        cache_.erase(it);
    }
    
    // 添加新条目
    currentMemoryUsage_ += entry.memorySize;
    cache_[filePath] = std::move(entry);
    
    LOG_DEBUG_FMT("添加到Go分析器缓存: %s, 内存占用: %zu 字节", filePath.c_str(), entry.memorySize);
}

void GoAnalyzer::evictLRUCache() const {
    if (cache_.empty()) {
        return;
    }
    
    // 找到最久未访问的条目
    auto lruIt = cache_.begin();
    std::time_t oldestTime = lruIt->second.accessTime;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.accessTime < oldestTime) {
            oldestTime = it->second.accessTime;
            lruIt = it;
        }
    }
    
    // 移除最久未访问的条目
    LOG_DEBUG_FMT("从Go分析器缓存中移除LRU条目: %s", lruIt->first.c_str());
    currentMemoryUsage_ -= lruIt->second.memorySize;
    cache_.erase(lruIt);
}

size_t GoAnalyzer::estimateMemoryUsage(const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& results) const {
    size_t totalSize = 0;
    
    for (const auto& result : results) {
        if (result) {
            // 基本结构大小
            totalSize += sizeof(ast_analyzer::ASTNodeInfo);
            
            // 字符串成员大小
            totalSize += result->name.size();
            totalSize += result->text.size();
            totalSize += result->location.filePath.size();
            totalSize += result->location.fileName.size();
            
            // 子节点大小（递归计算）
            totalSize += estimateMemoryUsage(result->children);
        }
    }
    
    return totalSize;
}

} // namespace analyzer
} // namespace core
} // namespace dlogcover 