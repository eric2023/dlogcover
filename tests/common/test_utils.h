/**
 * @file test_utils.h
 * @brief 测试工具类 - 提供通用的测试辅助功能
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_TESTS_COMMON_TEST_UTILS_H
#define DLOGCOVER_TESTS_COMMON_TEST_UTILS_H

#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <chrono>
#include <memory>
#include <vector>
#include <fstream>

// 添加必要的业务代码头文件
#include <dlogcover/config/config.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/source_manager/source_manager.h>

namespace dlogcover {
namespace tests {
namespace common {

/**
 * @brief 临时目录管理器
 * 
 * 自动创建和清理临时测试目录
 */
class TempDirectoryManager {
public:
    /**
     * @brief 构造函数，创建临时目录
     * @param prefix 目录前缀
     */
    explicit TempDirectoryManager(const std::string& prefix = "dlogcover_test");
    
    /**
     * @brief 析构函数，自动清理临时目录
     */
    ~TempDirectoryManager();
    
    /**
     * @brief 获取临时目录路径
     * @return 临时目录路径
     */
    const std::filesystem::path& getPath() const { return tempDir_; }
    
    /**
     * @brief 创建测试文件
     * @param filename 文件名
     * @param content 文件内容
     * @return 创建的文件路径
     */
    std::filesystem::path createTestFile(const std::string& filename, const std::string& content);
    
    /**
     * @brief 清理所有创建的文件
     */
    void cleanup();

private:
    std::filesystem::path tempDir_;
    std::vector<std::filesystem::path> createdFiles_;
};

/**
 * @brief 性能测试计时器
 */
class PerformanceTimer {
public:
    /**
     * @brief 构造函数，开始计时
     */
    PerformanceTimer();
    
    /**
     * @brief 获取经过的时间（毫秒）
     * @return 经过的时间
     */
    std::chrono::milliseconds elapsed() const;
    
    /**
     * @brief 重置计时器
     */
    void reset();

private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

/**
 * @brief 浮点数比较工具
 */
class FloatComparator {
public:
    /**
     * @brief 比较两个浮点数是否近似相等
     * @param a 第一个数
     * @param b 第二个数
     * @param tolerance 容差
     * @return 是否近似相等
     */
    static bool nearEqual(double a, double b, double tolerance = 1e-9);
    
    /**
     * @brief 比较两个浮点数是否近似相等（相对误差）
     * @param a 第一个数
     * @param b 第二个数
     * @param relativeError 相对误差
     * @return 是否近似相等
     */
    static bool relativeEqual(double a, double b, double relativeError = 1e-6);
};

/**
 * @brief 测试超时管理器
 */
class TimeoutManager {
public:
    /**
     * @brief 构造函数
     * @param timeoutMs 超时时间（毫秒）
     */
    explicit TimeoutManager(std::chrono::milliseconds timeoutMs);
    
    /**
     * @brief 检查是否超时
     * @return 是否超时
     */
    bool isTimeout() const;
    
    /**
     * @brief 获取剩余时间
     * @return 剩余时间（毫秒）
     */
    std::chrono::milliseconds remainingTime() const;

private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::milliseconds timeout_;
};

// 测试断言宏
#define EXPECT_NEAR_DOUBLE(a, b) \
    EXPECT_TRUE(dlogcover::tests::common::FloatComparator::nearEqual(a, b)) \
    << "Expected " << a << " to be near " << b

#define EXPECT_RELATIVE_EQUAL(a, b, error) \
    EXPECT_TRUE(dlogcover::tests::common::FloatComparator::relativeEqual(a, b, error)) \
    << "Expected " << a << " to be relatively equal to " << b << " with error " << error

#define EXPECT_TIMEOUT(timeout_ms, code) \
    do { \
        dlogcover::tests::common::TimeoutManager timeout_mgr(std::chrono::milliseconds(timeout_ms)); \
        code; \
        EXPECT_FALSE(timeout_mgr.isTimeout()) << "Code execution timed out after " << timeout_ms << "ms"; \
    } while(0)

} // namespace common
} // namespace tests

// 添加TestUtils类到test命名空间
namespace test {

/**
 * @brief 测试工具类 - 提供集成测试所需的辅助功能
 * 
 * 这个类提供了创建测试环境、配置和管理器的静态方法
 */
class TestUtils {
public:
    /**
     * @brief 创建测试临时目录
     * @param prefix 目录前缀
     * @return 创建的临时目录路径
     */
    static std::string createTestTempDir(const std::string& prefix = "test_");
    
    /**
     * @brief 创建测试配置
     * @param testDir 测试目录路径
     * @return 测试配置对象
     */
    static config::Config createTestConfig(const std::string& testDir);
    
    /**
     * @brief 创建测试源文件管理器
     * @param config 配置对象
     * @return 源文件管理器智能指针
     */
    static std::unique_ptr<source_manager::SourceManager> createTestSourceManager(const config::Config& config);
    
    /**
     * @brief 清理测试临时目录
     * @param path 要清理的目录路径
     * @return 清理是否成功
     */
    static bool cleanupTestTempDir(const std::string& path);

private:
    // 私有构造函数，防止实例化
    TestUtils() = delete;
    ~TestUtils() = delete;
    TestUtils(const TestUtils&) = delete;
    TestUtils& operator=(const TestUtils&) = delete;
};

} // namespace test
} // namespace dlogcover

#endif // DLOGCOVER_TESTS_COMMON_TEST_UTILS_H
