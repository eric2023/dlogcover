#pragma once

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>

#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>

namespace dlogcover {
namespace test {

class TestUtils {
public:
    // 创建临时测试目录
    static std::string createTestTempDir(const std::string& prefix = "test_",
                                         std::chrono::seconds timeout = std::chrono::seconds(5));

    // 清理临时测试目录
    static bool cleanupTestTempDir(const std::string& path, std::chrono::seconds timeout = std::chrono::seconds(5));

    // 创建测试配置
    static config::Config createTestConfig(const std::string& testDir);

    // 创建测试源码管理器
    static std::unique_ptr<source_manager::SourceManager> createTestSourceManager(const config::Config& config);

    // 收集并分析源代码
    static bool collectAndAnalyzeSource(source_manager::SourceManager& sourceManager,
                                        core::ast_analyzer::ASTAnalyzer& astAnalyzer);

    // 识别日志
    static bool identifyLogs(core::ast_analyzer::ASTAnalyzer& astAnalyzer,
                             core::log_identifier::LogIdentifier& logIdentifier);

    // 计算覆盖率
    static bool calculateCoverage(core::log_identifier::LogIdentifier& logIdentifier,
                                  core::ast_analyzer::ASTAnalyzer& astAnalyzer,
                                  core::coverage::CoverageCalculator& coverageCalculator);

    // 创建测试源文件
    static std::string createTestSourceFile(const std::string& dirPath, const std::string& filename,
                                            const std::string& content);

private:
    // 生成唯一的临时目录名
    static std::string generateUniqueDirName(const std::string& prefix);
};

}  // namespace test
}  // namespace dlogcover
