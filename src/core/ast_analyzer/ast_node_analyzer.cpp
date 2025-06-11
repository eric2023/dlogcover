/**
 * @file ast_node_analyzer.cpp
 * @brief AST节点分析器基类实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/ast_analyzer/ast_node_analyzer.h>
#include <dlogcover/utils/log_utils.h>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

Location ASTNodeAnalyzer::getLocation(clang::SourceLocation loc) const {
    Location location;
    location.filePath = filePath_;

    auto& SM = context_.getSourceManager();
    if (loc.isValid()) {
        location.line = SM.getSpellingLineNumber(loc);
        location.column = SM.getSpellingColumnNumber(loc);
    } else {
        location.line = 0;
        location.column = 0;
        LOG_DEBUG_FMT("无效的源代码位置: %s", filePath_.c_str());
    }

    return location;
}

std::string ASTNodeAnalyzer::getSourceText(clang::SourceLocation beginLoc, clang::SourceLocation endLoc,
                                           size_t maxLength) const {
    if (!beginLoc.isValid() || !endLoc.isValid()) {
        LOG_DEBUG_FMT("无效的源代码位置范围: %s", filePath_.c_str());
        return "";
    }

    auto& SM = context_.getSourceManager();
    auto beginOffset = SM.getFileOffset(beginLoc);
    auto endOffset = SM.getFileOffset(endLoc);

    if (beginOffset > endOffset) {
        LOG_DEBUG_FMT("源代码位置范围错误: %s", filePath_.c_str());
        return "";
    }

    auto length = endOffset - beginOffset;
    if (maxLength > 0 && length > maxLength) {
        length = maxLength;
    }

    bool invalid = false;
    auto text = std::string(SM.getCharacterData(beginLoc), length);
    if (invalid) {
        LOG_DEBUG_FMT("获取源代码文本失败: %s", filePath_.c_str());
        return "";
    }

    return text;
}

std::unique_ptr<ASTNodeInfo> ASTNodeAnalyzer::createNodeInfo(NodeType type, const std::string& name,
                                                             clang::SourceLocation loc, const std::string& text) const {
    auto nodeInfo = std::make_unique<ASTNodeInfo>();
    nodeInfo->type = type;
    nodeInfo->location = getLocation(loc);
    nodeInfo->name = name;
    nodeInfo->text = text;
    nodeInfo->hasLogging = false;  // 默认设置为false，由具体分析器设置

    LOG_DEBUG_FMT("创建节点信息: type=%d, name=%s, line=%u, column=%u", static_cast<int>(type), name.c_str(),
                  nodeInfo->location.line, nodeInfo->location.column);

    return nodeInfo;
}

// ASTNodeInfo拷贝构造函数和拷贝赋值运算符实现
ASTNodeInfo::ASTNodeInfo(const ASTNodeInfo& other) 
    : type(other.type)
    , name(other.name)
    , location(other.location)
    , endLocation(other.endLocation)
    , text(other.text)
    , hasLogging(other.hasLogging) {
    
    // 深拷贝子节点
    children.reserve(other.children.size());
    for (const auto& child : other.children) {
        if (child) {
            children.push_back(std::make_unique<ASTNodeInfo>(*child));
        }
    }
}

ASTNodeInfo& ASTNodeInfo::operator=(const ASTNodeInfo& other) {
    if (this != &other) {
        type = other.type;
        name = other.name;
        location = other.location;
        endLocation = other.endLocation;
        text = other.text;
        hasLogging = other.hasLogging;
        
        // 清空并深拷贝子节点
        children.clear();
        children.reserve(other.children.size());
        for (const auto& child : other.children) {
            if (child) {
                children.push_back(std::make_unique<ASTNodeInfo>(*child));
            }
        }
    }
    return *this;
}

}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
