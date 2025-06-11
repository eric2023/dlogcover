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
} // namespace dlogcover 