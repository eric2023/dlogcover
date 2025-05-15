/**
 * @file log_identifier.cpp
 * @brief 日志识别器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace log_identifier {

LogIdentifier::LogIdentifier(const config::Config& config, const ast_analyzer::ASTAnalyzer& astAnalyzer)
    : config_(config), astAnalyzer_(astAnalyzer) {
    LOG_DEBUG("日志识别器初始化");
    buildLogFunctionNameSet();
}

LogIdentifier::~LogIdentifier() {
    LOG_DEBUG("日志识别器销毁");
}

bool LogIdentifier::identifyLogCalls() {
    LOG_INFO("开始识别日志调用");

    logCalls_.clear();

    // 获取所有AST节点信息
    const auto& astNodes = astAnalyzer_.getAllASTNodeInfo();

    // 遍历所有文件的AST节点
    for (const auto& [filePath, nodeInfo] : astNodes) {
        LOG_INFO_FMT("识别文件中的日志调用: %s", filePath.c_str());

        // 初始化该文件的日志调用列表
        logCalls_[filePath] = std::vector<LogCallInfo>();

        // 递归识别节点中的日志调用
        identifyLogCallsInNode(nodeInfo.get(), filePath);

        LOG_INFO_FMT("文件 %s 中识别到 %zu 个日志调用", filePath.c_str(), logCalls_[filePath].size());
    }

    // 输出日志识别统计信息
    size_t totalLogCalls = 0;
    for (const auto& [filePath, calls] : logCalls_) {
        totalLogCalls += calls.size();
    }

    LOG_INFO_FMT("日志调用识别完成，共识别 %zu 个文件中的 %zu 个日志调用", logCalls_.size(), totalLogCalls);

    return !logCalls_.empty();
}

const std::vector<LogCallInfo>& LogIdentifier::getLogCalls(const std::string& filePath) const {
    static const std::vector<LogCallInfo> emptyLogCalls;

    auto it = logCalls_.find(filePath);
    if (it != logCalls_.end()) {
        return it->second;
    }

    return emptyLogCalls;
}

const std::unordered_map<std::string, std::vector<LogCallInfo>>& LogIdentifier::getAllLogCalls() const {
    return logCalls_;
}

const std::unordered_set<std::string>& LogIdentifier::getLogFunctionNames() const {
    return logFunctionNames_;
}

void LogIdentifier::buildLogFunctionNameSet() {
    LOG_DEBUG("构建日志函数名集合");

    logFunctionNames_.clear();
    functionNameToLevel_.clear();
    functionNameToType_.clear();

    // 添加Qt日志函数
    if (config_.logFunctions.qt.enabled) {
        for (const auto& func : config_.logFunctions.qt.functions) {
            logFunctionNames_.insert(func);

            // 根据函数名确定日志级别
            if (func == "qDebug") {
                functionNameToLevel_[func] = LogLevel::DEBUG;
            } else if (func == "qInfo") {
                functionNameToLevel_[func] = LogLevel::INFO;
            } else if (func == "qWarning") {
                functionNameToLevel_[func] = LogLevel::WARNING;
            } else if (func == "qCritical") {
                functionNameToLevel_[func] = LogLevel::CRITICAL;
            } else if (func == "qFatal") {
                functionNameToLevel_[func] = LogLevel::FATAL;
            } else {
                functionNameToLevel_[func] = LogLevel::UNKNOWN;
            }

            functionNameToType_[func] = LogType::QT;
        }

        // 添加Qt分类日志函数
        for (const auto& func : config_.logFunctions.qt.categoryFunctions) {
            logFunctionNames_.insert(func);

            // 根据函数名确定日志级别
            if (func == "qCDebug") {
                functionNameToLevel_[func] = LogLevel::DEBUG;
            } else if (func == "qCInfo") {
                functionNameToLevel_[func] = LogLevel::INFO;
            } else if (func == "qCWarning") {
                functionNameToLevel_[func] = LogLevel::WARNING;
            } else if (func == "qCCritical") {
                functionNameToLevel_[func] = LogLevel::CRITICAL;
            } else {
                functionNameToLevel_[func] = LogLevel::UNKNOWN;
            }

            functionNameToType_[func] = LogType::QT;
        }
    }

    // 添加自定义日志函数
    if (config_.logFunctions.custom.enabled) {
        for (const auto& [level, funcs] : config_.logFunctions.custom.functions) {
            for (const auto& func : funcs) {
                logFunctionNames_.insert(func);

                // 设置日志级别
                if (level == "debug") {
                    functionNameToLevel_[func] = LogLevel::DEBUG;
                } else if (level == "info") {
                    functionNameToLevel_[func] = LogLevel::INFO;
                } else if (level == "warning") {
                    functionNameToLevel_[func] = LogLevel::WARNING;
                } else if (level == "critical") {
                    functionNameToLevel_[func] = LogLevel::CRITICAL;
                } else if (level == "fatal") {
                    functionNameToLevel_[func] = LogLevel::FATAL;
                } else {
                    functionNameToLevel_[func] = LogLevel::UNKNOWN;
                }

                functionNameToType_[func] = LogType::CUSTOM;
            }
        }
    }

    LOG_INFO_FMT("共构建了 %zu 个日志函数名", logFunctionNames_.size());
}

void LogIdentifier::identifyLogCallsInNode(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath) {
    if (!node) {
        return;
    }

    // 检查当前节点是否为日志函数调用
    if (node->type == ast_analyzer::NodeType::LOG_CALL_EXPR) {
        LOG_DEBUG_FMT("找到日志函数调用: %s, 位置: %s:%d:%d", node->name.c_str(), node->location.filePath.c_str(),
                      node->location.line, node->location.column);

        // 创建日志调用信息
        LogCallInfo logCallInfo;
        logCallInfo.functionName = node->name;
        logCallInfo.location = node->location;

        // 提取日志消息
        logCallInfo.message = extractLogMessage(node);

        // 设置日志级别和类型
        logCallInfo.level = getLogLevel(node->name);
        logCallInfo.type = getLogType(node->name);

        // 添加到日志调用列表
        logCalls_[filePath].push_back(logCallInfo);
    }
    // 检查常规函数调用，判断是否为日志函数
    else if (node->type == ast_analyzer::NodeType::CALL_EXPR) {
        // 检查函数名是否在已知日志函数名集合中
        if (logFunctionNames_.find(node->name) != logFunctionNames_.end()) {
            LOG_DEBUG_FMT("在函数调用中识别到日志函数: %s, 位置: %s:%d:%d", node->name.c_str(),
                          node->location.filePath.c_str(), node->location.line, node->location.column);

            // 创建日志调用信息
            LogCallInfo logCallInfo;
            logCallInfo.functionName = node->name;
            logCallInfo.location = node->location;
            logCallInfo.message = extractLogMessage(node);
            logCallInfo.level = getLogLevel(node->name);
            logCallInfo.type = getLogType(node->name);

            // 添加到日志调用列表
            logCalls_[filePath].push_back(logCallInfo);
        }
    }

    // 递归处理子节点
    for (const auto& child : node->children) {
        identifyLogCallsInNode(child.get(), filePath);
    }
}

LogLevel LogIdentifier::getLogLevel(const std::string& functionName) const {
    auto it = functionNameToLevel_.find(functionName);
    if (it != functionNameToLevel_.end()) {
        return it->second;
    }
    return LogLevel::UNKNOWN;
}

LogType LogIdentifier::getLogType(const std::string& functionName) const {
    auto it = functionNameToType_.find(functionName);
    if (it != functionNameToType_.end()) {
        return it->second;
    }
    return LogType::UNKNOWN;
}

std::string LogIdentifier::extractLogMessage(const ast_analyzer::ASTNodeInfo* callExpr) const {
    if (!callExpr || callExpr->text.empty()) {
        return "";
    }

    // 简化实现
    // 在实际场景中，我们需要更复杂的解析来提取日志消息

    std::string text = callExpr->text;

    // 对于Qt日志，简单地查找第一个双引号之间的内容
    size_t firstQuote = text.find("\"");
    if (firstQuote != std::string::npos) {
        size_t secondQuote = text.find("\"", firstQuote + 1);
        if (secondQuote != std::string::npos) {
            return text.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        }
    }

    // 对于自定义日志格式，可以添加其他解析逻辑
    // 例如解析日志级别、类别等信息

    return "";
}

}  // namespace log_identifier
}  // namespace core
}  // namespace dlogcover
