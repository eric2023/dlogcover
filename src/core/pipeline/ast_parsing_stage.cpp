/**
 * @file ast_parsing_stage.cpp
 * @brief AST解析阶段实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/pipeline/ast_parsing_stage.h>
#include <dlogcover/utils/log_utils.h>
#include <dlogcover/utils/file_utils.h>
#include <fstream>
#include <sstream>

// Clang headers
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/AST/RecursiveASTVisitor.h>

namespace dlogcover {
namespace core {
namespace pipeline {

/**
 * @brief AST访问器，用于收集基本信息
 */
class BasicASTVisitor : public clang::RecursiveASTVisitor<BasicASTVisitor> {
public:
    explicit BasicASTVisitor(std::vector<std::string>& deps) : dependencies_(deps) {}
    
    // 简化依赖收集 - 在实际实现中会通过预处理器回调获取
    bool VisitFunctionDecl(clang::FunctionDecl* func) {
        // 作为示例，这里不收集依赖
        return true;
    }
    
private:
    std::vector<std::string>& dependencies_;
};

// ASTParsingStage 实现

ASTParsingStage::ASTParsingStage(const config::Config& config,
                               size_t max_queue_size,
                               size_t num_workers)
    : PipelineStage("AST-Parsing", max_queue_size, num_workers)
    , config_(config)
    , cache_enabled_(true) {
    
    LOG_INFO_FMT("AST解析阶段初始化：队列大小=%zu, 工作线程=%zu", 
                max_queue_size, num_workers);
}

std::shared_ptr<ASTParsingStage::OutputPacket> 
ASTParsingStage::processPacket(std::shared_ptr<InputPacket> input) {
    if (!input || !input->data) {
        LOG_WARNING("收到空的输入数据包");
        return nullptr;
    }
    
    const auto& source_info = *input->data;
    LOG_DEBUG_FMT("开始解析文件: %s", source_info.file_path.c_str());
    
    // 首先检查缓存
    if (cache_enabled_) {
        auto cached = getFromCache(source_info.file_path);
        if (cached) {
            files_cached_.fetch_add(1);
            LOG_DEBUG_FMT("缓存命中: %s", source_info.file_path.c_str());
            return std::make_shared<OutputPacket>(cached, source_info.file_path);
        }
    }
    
    // 解析文件
    auto parsed_ast = parseSourceFile(source_info);
    if (!parsed_ast) {
        parse_errors_.fetch_add(1);
        LOG_ERROR_FMT("文件解析失败: %s", source_info.file_path.c_str());
        return nullptr;
    }
    
    // 缓存结果
    if (cache_enabled_ && parsed_ast->parse_success) {
        cacheASTResult(parsed_ast);
    }
    
    files_parsed_.fetch_add(1);
    auto current_time = total_parse_time_.load();
    total_parse_time_.store(current_time + parsed_ast->getParsingTimeMs());
    
    LOG_DEBUG_FMT("文件解析完成: %s, 耗时: %.2fms", 
                  source_info.file_path.c_str(), parsed_ast->getParsingTimeMs());
    
    return std::make_shared<OutputPacket>(parsed_ast, source_info.file_path);
}

void ASTParsingStage::onStart() {
    LOG_INFO_FMT("AST解析阶段启动，工作线程数: %zu", num_workers_);
    
    // 重置统计信息
    files_parsed_.store(0);
    files_cached_.store(0);
    parse_errors_.store(0);
    total_parse_time_.store(0.0);
}

void ASTParsingStage::onStop() {
    LOG_INFO_FMT("AST解析阶段停止，统计信息: %s", getParsingStats().c_str());
}

std::shared_ptr<ParsedASTInfo> 
ASTParsingStage::parseSourceFile(const SourceFileInfo& source_info) {
    auto result = std::make_shared<ParsedASTInfo>(source_info.file_path);
    result->parse_start = std::chrono::steady_clock::now();
    
    try {
        // 构建编译参数
        auto compile_args = buildCompileArgs(source_info);
        
        // 创建AST单元
        std::vector<std::string> args_for_clang;
        args_for_clang.reserve(compile_args.size() + 1);
        args_for_clang.push_back("dlogcover"); // 程序名
        args_for_clang.insert(args_for_clang.end(), compile_args.begin(), compile_args.end());
        
        // 使用Clang工具链解析
                 // 读取文件内容
         std::string file_content = source_info.content;
         if (file_content.empty()) {
             if (!utils::FileUtils::readFileToString(source_info.file_path, file_content)) {
                 throw std::runtime_error("无法读取文件: " + source_info.file_path);
             }
         }
         
         result->ast_unit = clang::tooling::buildASTFromCodeWithArgs(
             file_content,
             args_for_clang,
             source_info.file_path
         );
        
        if (result->ast_unit) {
            result->parse_success = true;
            
            // 扫描依赖关系
            if (result->ast_unit->getASTContext().getTranslationUnitDecl()) {
                result->dependencies = scanDependencies(result->ast_unit->getASTContext());
            }
            
                         // 创建根节点信息（简化版）
             result->root_node = std::make_unique<ast_analyzer::ASTNodeInfo>();
             result->root_node->type = ast_analyzer::NodeType::DECLARATION;
             result->root_node->name = utils::FileUtils::getFileName(source_info.file_path);
            
            LOG_DEBUG_FMT("AST解析成功: %s, 依赖数: %zu", 
                          source_info.file_path.c_str(), result->dependencies.size());
        } else {
            result->parse_success = false;
            result->error_message = "AST单元创建失败";
            LOG_WARNING_FMT("AST解析失败: %s", source_info.file_path.c_str());
        }
        
    } catch (const std::exception& e) {
        result->parse_success = false;
        result->error_message = e.what();
        LOG_ERROR_FMT("AST解析异常: %s, 错误: %s", 
                      source_info.file_path.c_str(), e.what());
    }
    
    result->parse_end = std::chrono::steady_clock::now();
    return result;
}

std::shared_ptr<ParsedASTInfo> 
ASTParsingStage::getFromCache(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = ast_cache_.find(file_path);
    if (it != ast_cache_.end()) {
        // 简单的时间戳检查（生产环境中应该更复杂）
                 // 简化处理：暂时不检查时间戳，直接返回缓存
         return it->second;
    }
    
    return nullptr;
}

void ASTParsingStage::cacheASTResult(std::shared_ptr<ParsedASTInfo> ast_info) {
    if (!ast_info || !ast_info->parse_success) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 如果缓存满了，移除最旧的条目（简化的LRU）
    if (ast_cache_.size() >= MAX_CACHE_SIZE) {
        auto oldest = ast_cache_.begin();
        ast_cache_.erase(oldest);
    }
    
    ast_cache_[ast_info->file_path] = ast_info;
}

std::vector<std::string> 
ASTParsingStage::scanDependencies(clang::ASTContext& ast_context) {
    std::vector<std::string> dependencies;
    
    try {
        // 简化的依赖扫描，实际实现会更复杂
        BasicASTVisitor visitor(dependencies);
        visitor.TraverseDecl(ast_context.getTranslationUnitDecl());
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("依赖扫描失败: %s", e.what());
    }
    
    return dependencies;
}

std::vector<std::string> 
ASTParsingStage::buildCompileArgs(const SourceFileInfo& source_info) {
    std::vector<std::string> args;
    
    // 使用提供的编译参数
    if (!source_info.compile_args.empty()) {
        args = source_info.compile_args;
    } else {
        // 默认编译参数
        args = {
            "-std=c++17",
            "-I/usr/include",
            "-I/usr/local/include"
        };
        
        // 添加项目包含路径
        if (!config_.project.directory.empty()) {
            args.push_back("-I" + config_.project.directory + "/include");
        }
    }
    
    return args;
}

size_t ASTParsingStage::estimateFileComplexity(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return 0;
        }
        
        size_t line_count = 0;
        std::string line;
        while (std::getline(file, line)) {
            line_count++;
        }
        
        return line_count;
    } catch (const std::exception&) {
        return 0;
    }
}

std::string ASTParsingStage::getParsingStats() const {
    std::ostringstream stats;
    stats << "AST解析统计: ";
    stats << "已解析=" << files_parsed_.load();
    stats << ", 缓存命中=" << files_cached_.load();
    stats << ", 解析错误=" << parse_errors_.load();
    stats << ", 平均耗时=" << (files_parsed_.load() > 0 ? 
                             total_parse_time_.load() / files_parsed_.load() : 0.0) << "ms";
    return stats.str();
}

// FunctionDecompositionStage 实现

FunctionDecompositionStage::FunctionDecompositionStage(size_t max_queue_size,
                                                       size_t num_workers)
    : PipelineStage("Function-Decomposition", max_queue_size, num_workers) {
    
    LOG_INFO_FMT("函数分解阶段初始化：队列大小=%zu, 工作线程=%zu", 
                max_queue_size, num_workers);
}

std::shared_ptr<FunctionDecompositionStage::OutputPacket>
FunctionDecompositionStage::processPacket(std::shared_ptr<InputPacket> input) {
    if (!input || !input->data) {
        return nullptr;
    }
    
    const auto& ast_info = *input->data;
    if (!ast_info.parse_success || !ast_info.ast_unit) {
        LOG_WARNING_FMT("跳过解析失败的文件: %s", ast_info.file_path.c_str());
        return nullptr;
    }
    
    // 提取函数
    auto functions = extractFunctions(ast_info);
    LOG_DEBUG_FMT("从文件 %s 提取到 %zu 个函数", 
                  ast_info.file_path.c_str(), functions.size());
    
    // 为每个函数创建任务（这里只返回第一个，实际实现需要批量处理）
    if (!functions.empty()) {
        auto func_decl = functions[0]; // 简化处理
        auto task = std::make_shared<FunctionTask>(
            func_decl->getNameAsString(),
            ast_info.file_path,
            func_decl,
            std::const_pointer_cast<ParsedASTInfo>(input->data)
        );
        
        task->estimated_complexity = calculateFunctionComplexity(func_decl);
        task->priority = determineFunctionPriority(func_decl, task->estimated_complexity);
        
        files_processed_.fetch_add(1);
        functions_extracted_.fetch_add(functions.size());
        
        if (task->estimated_complexity > 50) { // 阈值
            complex_functions_.fetch_add(1);
        }
        
        return std::make_shared<OutputPacket>(task, ast_info.file_path);
    }
    
    return nullptr;
}

std::vector<clang::FunctionDecl*> 
FunctionDecompositionStage::extractFunctions(const ParsedASTInfo& ast_info) {
    std::vector<clang::FunctionDecl*> functions;
    
    if (!ast_info.ast_unit) {
        return functions;
    }
    
    // 简化的函数提取（实际实现会使用RecursiveASTVisitor）
    auto* tu_decl = ast_info.ast_unit->getASTContext().getTranslationUnitDecl();
    if (tu_decl) {
        for (auto* decl : tu_decl->decls()) {
            if (auto* func_decl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
                if (func_decl->hasBody()) {
                    functions.push_back(func_decl);
                }
            }
        }
    }
    
    return functions;
}

size_t FunctionDecompositionStage::calculateFunctionComplexity(clang::FunctionDecl* func_decl) {
    if (!func_decl || !func_decl->hasBody()) {
        return 0;
    }
    
    // 简化的复杂度计算（实际实现会更复杂）
    size_t complexity = 1; // 基础复杂度
    
    // 这里应该遍历函数体，计算分支、循环等
    // 暂时用参数数量作为简单指标
    complexity += func_decl->getNumParams();
    
    return complexity;
}

int FunctionDecompositionStage::determineFunctionPriority(clang::FunctionDecl* func_decl, 
                                                          size_t complexity) {
    int priority = 0;
    
    // 复杂度高的函数优先级高
    priority += static_cast<int>(complexity);
    
         // 构造函数和析构函数优先级较高
     if (llvm::dyn_cast<clang::CXXConstructorDecl>(func_decl)) {
         priority += 10;
     } else if (llvm::dyn_cast<clang::CXXDestructorDecl>(func_decl)) {
         priority += 15;
     }
    
    // 主函数最高优先级
    if (func_decl->getNameAsString() == "main") {
        priority += 100;
    }
    
    return priority;
}

std::string FunctionDecompositionStage::getDecompositionStats() const {
    std::ostringstream stats;
    stats << "函数分解统计: ";
    stats << "已处理文件=" << files_processed_.load();
    stats << ", 提取函数=" << functions_extracted_.load();
    stats << ", 复杂函数=" << complex_functions_.load();
    return stats.str();
}

} // namespace pipeline
} // namespace core
} // namespace dlogcover 