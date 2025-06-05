/**
 * @file reporter_test.cpp
 * @brief 报告生成器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/reporter/reporter.h>
#include <dlogcover/reporter/reporter_factory.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace dlogcover {
namespace reporter {
namespace test {

// 创建测试目录和文件的助手函数
class ReporterTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化日志系统，设置为ERROR级别以减少测试期间的日志输出
        utils::Logger::init("", false, utils::LogLevel::ERROR);
        
        // 创建测试目录
        testDir_ = "/tmp/dlogcover_reporter_test";
        utils::FileUtils::createDirectory(testDir_);
        outputDir_ = testDir_ + "/output";
        utils::FileUtils::createDirectory(outputDir_);

        // 创建测试文件
        createTestFile(testDir_ + "/test.cpp", R"(
#include <iostream>
#include <QDebug>

// 普通函数
void regular_function() {
    std::cout << "普通函数" << std::endl;
}

// 带日志的函数
void logged_function() {
    qDebug() << "这是一条调试日志";
    std::cout << "带日志的函数" << std::endl;
    qInfo() << "这是一条信息日志";
}

// 带条件分支的函数
int conditional_function(int value) {
    if (value > 0) {
        qDebug() << "正数分支" << value;
        return value * 2;
    } else {
        qWarning() << "负数或零分支" << value;
        return value * -1;
    }
}

// 带异常处理的函数
void exception_function() {
    try {
        throw std::runtime_error("测试异常");
    } catch (const std::exception& e) {
        qCritical() << "捕获异常:" << e.what();
        std::cerr << "捕获异常: " << e.what() << std::endl;
    }
}

int main() {
    regular_function();
    logged_function();
    conditional_function(10);
    conditional_function(-5);

    try {
        exception_function();
    } catch (...) {
        qFatal() << "致命错误";
    }

    return 0;
}
)");

        // 设置配置
        config_ = createTestConfig();

        // 初始化源文件管理器
        sourceManager_ = std::make_unique<source_manager::SourceManager>(config_);

        // 收集源文件
        auto collectResult = sourceManager_->collectSourceFiles();
        ASSERT_TRUE(collectResult) << "未能有效收集源文件";

        // 创建AST分析器
        astAnalyzer_ = std::make_unique<core::ast_analyzer::ASTAnalyzer>(config_, *sourceManager_);

        // 分析所有文件
        ASSERT_TRUE(astAnalyzer_->analyzeAll()) << "分析所有文件失败";

        // 创建日志识别器
        logIdentifier_ = std::make_unique<core::log_identifier::LogIdentifier>(config_, *astAnalyzer_);

        // 识别日志调用
        ASSERT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

        // 创建覆盖率计算器
        coverageCalculator_ =
            std::make_unique<core::coverage::CoverageCalculator>(config_, *astAnalyzer_, *logIdentifier_);

        // 计算覆盖率
        ASSERT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

        // 创建报告生成器
        reporter_ = std::make_unique<Reporter>(config_, *coverageCalculator_);

        // 创建进度回调
        progressCallback_ = [this](float progress, const std::string& message) {
            progressUpdates_.push_back({progress, message});
        };
    }

    void TearDown() override {
        // 关闭日志系统，确保所有资源正确释放
        utils::Logger::shutdown();
        
        // 清理测试目录
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
        reporter_.reset();
        coverageCalculator_.reset();
        logIdentifier_.reset();
        astAnalyzer_.reset();
        sourceManager_.reset();
    }

    void createTestFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    config::Config createTestConfig() {
        config::Config config;

        // 设置扫描目录
        config.scan.directories = {testDir_};

        // 设置文件类型
        config.scan.fileTypes = {".cpp", ".h", ".hpp", ".cc", ".c"};

        // 设置日志函数
        config.logFunctions.qt.enabled = true;
        config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};

        // 添加必要的编译参数
        config.scan.compilerArgs = {"-I/usr/include", "-I/usr/include/c++/8", "-I/usr/include/x86_64-linux-gnu/c++/8",
                                    "-I/usr/include/x86_64-linux-gnu", "-I/usr/local/include",
                                    // Qt头文件路径
                                    "-I/usr/include/x86_64-linux-gnu/qt5", "-I/usr/include/x86_64-linux-gnu/qt5/QtCore",
                                    "-I/usr/include/x86_64-linux-gnu/qt5/QtGui",
                                    "-I/usr/include/x86_64-linux-gnu/qt5/QtWidgets",
                                    // 系统定义
                                    "-D__GNUG__", "-D__linux__", "-D__x86_64__"};

        // 设置为Qt项目
        config.scan.isQtProject = true;

        // 设置分析选项
        config.analysis.functionCoverage = true;
        config.analysis.branchCoverage = true;
        config.analysis.exceptionCoverage = true;
        config.analysis.keyPathCoverage = true;

        // 设置报告选项
        config.report.format = "text";  // 默认文本格式

        return config;
    }

    // 用于保存进度更新的结构
    struct ProgressUpdate {
        float progress;
        std::string message;
    };

    std::string testDir_;
    std::string outputDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<core::ast_analyzer::ASTAnalyzer> astAnalyzer_;
    std::unique_ptr<core::log_identifier::LogIdentifier> logIdentifier_;
    std::unique_ptr<core::coverage::CoverageCalculator> coverageCalculator_;
    std::unique_ptr<Reporter> reporter_;
    ProgressCallback progressCallback_;
    std::vector<ProgressUpdate> progressUpdates_;
};

// 测试初始化和销毁
TEST_F(ReporterTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试文本格式报告生成
TEST_F(ReporterTestFixture, GenerateTextReport) {
    // 设置文本格式报告
    config_.report.format = "text";
    reporter_->setStrategy(ReporterFactory::getInstance().createStrategy(ReportFormat::TEXT));

    // 输出路径
    std::string outputPath = outputDir_ + "/coverage_report.txt";

    // 生成报告
    auto result = reporter_->generateReport(outputPath, progressCallback_);
    EXPECT_TRUE(result) << "生成文本报告失败";

    // 验证报告文件是否存在
    EXPECT_TRUE(utils::FileUtils::fileExists(outputPath)) << "报告文件不存在";

    // 读取文件内容进行简单验证
    std::string content;
    ASSERT_TRUE(utils::FileUtils::readFile(outputPath, content)) << "无法读取报告文件";

    // 验证报告内容
    EXPECT_TRUE(content.find("DLogCover 日志覆盖率报告") != std::string::npos) << "报告标题不正确";
    EXPECT_TRUE(content.find("总体覆盖率") != std::string::npos) << "缺少总体覆盖率部分";
    EXPECT_TRUE(content.find("文件覆盖率") != std::string::npos) << "缺少文件覆盖率部分";
    EXPECT_TRUE(content.find("改进建议") != std::string::npos) << "缺少改进建议部分";

    // 验证进度回调是否被调用
    EXPECT_FALSE(progressUpdates_.empty()) << "进度回调未被调用";
    EXPECT_GE(progressUpdates_.size(), 2) << "进度回调调用次数过少";
    EXPECT_FLOAT_EQ(progressUpdates_.front().progress, 0.1f) << "首次进度不正确";
    EXPECT_FLOAT_EQ(progressUpdates_.back().progress, 1.0f) << "最终进度不为100%";
}

// 测试JSON格式报告生成
TEST_F(ReporterTestFixture, GenerateJsonReport) {
    // 设置JSON格式报告
    config_.report.format = "json";
    reporter_->setStrategy(ReporterFactory::getInstance().createStrategy(ReportFormat::JSON));

    // 输出路径
    std::string outputPath = outputDir_ + "/coverage_report.json";

    // 生成报告
    auto result = reporter_->generateReport(outputPath, progressCallback_);
    EXPECT_TRUE(result) << "生成JSON报告失败";

    // 验证报告文件是否存在
    EXPECT_TRUE(utils::FileUtils::fileExists(outputPath)) << "报告文件不存在";

    // 读取文件内容进行简单验证
    std::string content;
    ASSERT_TRUE(utils::FileUtils::readFile(outputPath, content)) << "无法读取报告文件";

    // 解析JSON
    bool validJson = true;
    try {
        nlohmann::json reportJson = nlohmann::json::parse(content);

        // 验证JSON结构
        EXPECT_TRUE(reportJson.find("metadata") != reportJson.end()) << "缺少元数据部分";
        EXPECT_TRUE(reportJson.find("overall") != reportJson.end()) << "缺少总体统计部分";
        EXPECT_TRUE(reportJson.find("files") != reportJson.end()) << "缺少文件统计部分";

        // 验证总体覆盖率指标
        EXPECT_TRUE(reportJson["overall"].find("function_coverage") != reportJson["overall"].end())
            << "缺少函数覆盖率指标";
        EXPECT_TRUE(reportJson["overall"].find("branch_coverage") != reportJson["overall"].end())
            << "缺少分支覆盖率指标";
        EXPECT_TRUE(reportJson["overall"].find("exception_coverage") != reportJson["overall"].end())
            << "缺少异常覆盖率指标";
    } catch (const std::exception& e) {
        validJson = false;
        ADD_FAILURE() << "JSON解析失败: " << e.what();
    }

    EXPECT_TRUE(validJson) << "生成的JSON报告格式不正确";

    // 验证进度回调是否被调用
    EXPECT_FALSE(progressUpdates_.empty()) << "进度回调未被调用";
    EXPECT_GE(progressUpdates_.size(), 2) << "进度回调调用次数过少";
    EXPECT_FLOAT_EQ(progressUpdates_.front().progress, 0.1f) << "首次进度不正确";
    EXPECT_FLOAT_EQ(progressUpdates_.back().progress, 1.0f) << "最终进度不为100%";
}

// 测试多种格式报告生成
TEST_F(ReporterTestFixture, GenerateMultipleFormatReports) {
    // 生成文本报告
    std::string textOutputPath = outputDir_ + "/coverage_report.txt";
    auto textResult = reporter_->generateReport(textOutputPath, ReportFormat::TEXT);
    EXPECT_TRUE(textResult) << "生成文本报告失败";

    // 生成JSON报告
    std::string jsonOutputPath = outputDir_ + "/coverage_report.json";
    auto jsonResult = reporter_->generateReport(jsonOutputPath, ReportFormat::JSON);
    EXPECT_TRUE(jsonResult) << "生成JSON报告失败";

    // 验证文件存在
    EXPECT_TRUE(utils::FileUtils::fileExists(textOutputPath)) << "文本报告文件不存在";
    EXPECT_TRUE(utils::FileUtils::fileExists(jsonOutputPath)) << "JSON报告文件不存在";
}

// 测试报告生成器工厂
TEST_F(ReporterTestFixture, ReporterFactory) {
    // 获取工厂实例
    auto& factory = ReporterFactory::getInstance();

    // 测试创建文本策略
    auto textStrategy = factory.createStrategy(ReportFormat::TEXT);
    EXPECT_NE(textStrategy, nullptr) << "无法创建文本报告策略";
    EXPECT_EQ(textStrategy->getFormat(), ReportFormat::TEXT) << "文本报告策略格式不正确";
    EXPECT_EQ(textStrategy->getName(), "Text") << "文本报告策略名称不正确";

    // 测试创建JSON策略
    auto jsonStrategy = factory.createStrategy(ReportFormat::JSON);
    EXPECT_NE(jsonStrategy, nullptr) << "无法创建JSON报告策略";
    EXPECT_EQ(jsonStrategy->getFormat(), ReportFormat::JSON) << "JSON报告策略格式不正确";
    EXPECT_EQ(jsonStrategy->getName(), "JSON") << "JSON报告策略名称不正确";

    // 测试从字符串创建策略
    auto textStrategyFromStr = factory.createStrategy("text");
    EXPECT_NE(textStrategyFromStr, nullptr) << "无法从字符串创建文本报告策略";
    EXPECT_EQ(textStrategyFromStr->getFormat(), ReportFormat::TEXT) << "从字符串创建的文本报告策略格式不正确";

    // 测试获取支持的格式
    auto formats = factory.getSupportedFormats();
    EXPECT_FALSE(formats.empty()) << "支持的格式列表为空";
    EXPECT_GE(formats.size(), 2) << "支持的格式数量不足";
}

}  // namespace test
}  // namespace reporter
}  // namespace dlogcover
