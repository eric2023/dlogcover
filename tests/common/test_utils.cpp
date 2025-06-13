/**
 * @file test_utils.cpp
 * @brief 测试工具类实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include "test_utils.h"
#include <random>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace dlogcover {
namespace tests {
namespace common {

// TempDirectoryManager 实现
TempDirectoryManager::TempDirectoryManager(const std::string& prefix) {
    // 使用系统临时目录，确保跨平台兼容性
    auto tempBase = std::filesystem::temp_directory_path();
    
    // 生成唯一的目录名
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::string uniqueName = prefix + "_" + std::to_string(dis(gen));
    tempDir_ = tempBase / uniqueName;
    
    // 创建目录
    std::filesystem::create_directories(tempDir_);
}

TempDirectoryManager::~TempDirectoryManager() {
    cleanup();
}

std::filesystem::path TempDirectoryManager::createTestFile(const std::string& filename, const std::string& content) {
    auto filePath = tempDir_ / filename;
    
    // 确保父目录存在
    std::filesystem::create_directories(filePath.parent_path());
    
    // 创建文件
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << content;
        file.close();
        createdFiles_.push_back(filePath);
    }
    
    return filePath;
}

void TempDirectoryManager::cleanup() {
    try {
        if (std::filesystem::exists(tempDir_)) {
            std::filesystem::remove_all(tempDir_);
        }
        createdFiles_.clear();
    } catch (const std::exception&) {
        // 忽略清理错误，避免测试失败
    }
}

// PerformanceTimer 实现
PerformanceTimer::PerformanceTimer() {
    reset();
}

std::chrono::milliseconds PerformanceTimer::elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
}

void PerformanceTimer::reset() {
    startTime_ = std::chrono::high_resolution_clock::now();
}

// FloatComparator 实现
bool FloatComparator::nearEqual(double a, double b, double tolerance) {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }
    
    if (std::isinf(a) || std::isinf(b)) {
        return a == b;
    }
    
    return std::abs(a - b) <= tolerance;
}

bool FloatComparator::relativeEqual(double a, double b, double relativeError) {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }
    
    if (std::isinf(a) || std::isinf(b)) {
        return a == b;
    }
    
    if (a == b) {
        return true;
    }
    
    double maxValue = std::max(std::abs(a), std::abs(b));
    if (maxValue == 0.0) {
        return true;
    }
    
    return std::abs(a - b) / maxValue <= relativeError;
}

// TimeoutManager 实现
TimeoutManager::TimeoutManager(std::chrono::milliseconds timeoutMs) 
    : timeout_(timeoutMs) {
    startTime_ = std::chrono::high_resolution_clock::now();
}

bool TimeoutManager::isTimeout() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
    return elapsed >= timeout_;
}

std::chrono::milliseconds TimeoutManager::remainingTime() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
    auto remaining = timeout_ - elapsed;
    return remaining.count() > 0 ? remaining : std::chrono::milliseconds(0);
}

} // namespace common
} // namespace tests

// 实现TestUtils类
namespace test {

std::string TestUtils::createTestTempDir(const std::string& prefix) {
    // 使用现有的TempDirectoryManager来创建临时目录
    static std::vector<std::unique_ptr<tests::common::TempDirectoryManager>> tempDirs;
    
    auto tempDirManager = std::make_unique<tests::common::TempDirectoryManager>(prefix);
    std::string path = tempDirManager->getPath().string();
    
    // 保存管理器实例以便后续清理
    tempDirs.push_back(std::move(tempDirManager));
    
    return path;
}

config::Config TestUtils::createTestConfig(const std::string& testDir) {
    config::Config config;
    
    // 设置项目配置
    config.project.name = "test_project";
    config.project.directory = testDir;
    config.project.build_directory = testDir + "/build";
    
    // 设置扫描配置
    config.scan.directories = {"src"};
    config.scan.file_extensions = {".cpp", ".h", ".hpp", ".cc", ".cxx"};
    
    // 设置编译命令配置
    config.compile_commands.path = testDir + "/compile_commands.json";
    config.compile_commands.auto_generate = true;
    
    // 设置输出配置
    config.output.report_file = testDir + "/output/report.txt";
    config.output.log_file = testDir + "/output/test.log";
    config.output.log_level = "INFO";
    
    // 设置日志函数配置（Qt日志函数默认启用）
    config.log_functions.qt.enabled = true;
    config.log_functions.custom.enabled = true;
    
    // 设置分析配置
    config.analysis.function_coverage = true;
    config.analysis.branch_coverage = true;
    config.analysis.exception_coverage = true;
    config.analysis.key_path_coverage = true;
    
    // 设置性能配置（测试时禁用并行以避免复杂性）
    config.performance.enable_parallel_analysis = false;
    config.performance.max_threads = 1;
    config.performance.enable_ast_cache = true;
    
    // 创建必要的目录
    std::filesystem::create_directories(testDir + "/src");
    std::filesystem::create_directories(testDir + "/output");
    std::filesystem::create_directories(testDir + "/build");
    
    return config;
}

std::unique_ptr<source_manager::SourceManager> TestUtils::createTestSourceManager(const config::Config& config) {
    // 创建源文件管理器
    auto sourceManager = std::make_unique<source_manager::SourceManager>(config);
    
    return sourceManager;
}

bool TestUtils::cleanupTestTempDir(const std::string& path) {
    try {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove_all(path);
            return true;
        }
        return true; // 目录不存在也算成功
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace test
} // namespace dlogcover 