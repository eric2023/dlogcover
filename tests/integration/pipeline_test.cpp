/**
 * @file pipeline_test.cpp
 * @brief 流水线集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/pipeline/pipeline_manager.h>
#include <dlogcover/config/config_manager.h>
#include <fstream>
#include <filesystem>

using namespace dlogcover::core::pipeline;
using namespace dlogcover::config;

class PipelineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试文件
        test_file_path_ = "test_pipeline_file.cpp";
        createTestFile();
        
        // 创建配置
        config_manager_ = std::make_unique<ConfigManager>();
        config_ = config_manager_->getConfig();
        
        // 配置流水线
        pipeline_config_.ast_parsing_workers = 1;
        pipeline_config_.function_decomposition_workers = 1;
        pipeline_config_.function_analysis_workers = 2;
        pipeline_config_.ast_parsing_queue_size = 10;
        pipeline_config_.function_decomposition_queue_size = 20;
        pipeline_config_.function_analysis_queue_size = 30;
    }
    
    void TearDown() override {
        // 清理测试文件
        if (std::filesystem::exists(test_file_path_)) {
            std::filesystem::remove(test_file_path_);
        }
    }
    
    void createTestFile() {
        std::ofstream file(test_file_path_);
        file << R"(
#include <iostream>
#include <string>

void simpleFunction() {
    std::cout << "Hello World" << std::endl;
    LOG_INFO("This is a log message");
}

int complexFunction(int a, int b) {
    LOG_DEBUG_FMT("Processing values: a=%d, b=%d", a, b);
    
    if (a > b) {
        LOG_WARNING("a is greater than b");
        return a;
    } else {
        LOG_ERROR("b is greater or equal to a");
        return b;
    }
}

class TestClass {
public:
    TestClass() {
        LOG_INFO("TestClass constructor");
    }
    
    ~TestClass() {
        LOG_INFO("TestClass destructor");
    }
    
    void methodWithException() {
        try {
            LOG_DEBUG("Attempting risky operation");
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            LOG_ERROR_FMT("Caught exception: %s", e.what());
        }
    }
};

int main() {
    LOG_INFO("Program starting");
    
    simpleFunction();
    
    int result = complexFunction(10, 5);
    LOG_INFO_FMT("Result: %d", result);
    
    TestClass test;
    test.methodWithException();
    
    LOG_INFO("Program ending");
    return 0;
}
)";
        file.close();
    }
    
    std::string test_file_path_;
    std::unique_ptr<ConfigManager> config_manager_;
    Config config_;
    PipelineConfig pipeline_config_;
};

TEST_F(PipelineIntegrationTest, BasicPipelineExecution) {
    // 创建流水线管理器
    auto manager = std::make_unique<PipelineManager>(config_, pipeline_config_);
    
    // 启动流水线
    ASSERT_TRUE(manager->start()) << "流水线启动失败";
    EXPECT_TRUE(manager->isRunning()) << "流水线应该处于运行状态";
    
    // 处理测试文件
    bool file_submitted = manager->processFile(test_file_path_);
    EXPECT_TRUE(file_submitted) << "文件提交失败";
    
    // 等待处理完成
    bool completed = manager->waitForCompletion(5000); // 5秒超时
    EXPECT_TRUE(completed) << "流水线处理超时";
    
    // 获取结果
    auto results = manager->getCurrentResults();
    
    // 验证结果
    EXPECT_GT(results.total_functions_analyzed, 0) << "应该分析到一些函数";
    
    // 停止流水线
    manager->stop();
    EXPECT_FALSE(manager->isRunning()) << "流水线应该已停止";
}

TEST_F(PipelineIntegrationTest, BatchFileProcessing) {
    // 创建多个测试文件
    std::vector<std::string> test_files;
    for (int i = 0; i < 3; ++i) {
        std::string file_path = "test_batch_" + std::to_string(i) + ".cpp";
        test_files.push_back(file_path);
        
        std::ofstream file(file_path);
        file << "#include <iostream>\n";
        file << "void function" << i << "() {\n";
        file << "    LOG_INFO(\"Function " << i << "\");\n";
        file << "    std::cout << \"Function " << i << "\" << std::endl;\n";
        file << "}\n";
        file.close();
    }
    
    // 创建流水线管理器
    auto manager = std::make_unique<PipelineManager>(config_, pipeline_config_);
    ASSERT_TRUE(manager->start());
    
    // 批量处理文件
    auto future_results = manager->processFiles(test_files);
    
    // 等待结果
    auto future_status = future_results.wait_for(std::chrono::seconds(10));
    ASSERT_EQ(future_status, std::future_status::ready) << "批量处理超时";
    
    auto results = future_results.get();
    
    // 验证结果
    EXPECT_EQ(results.total_files_processed, test_files.size()) << "处理的文件数不匹配";
    EXPECT_GT(results.total_functions_analyzed, 0) << "应该分析到一些函数";
    
    // 清理测试文件
    for (const auto& file_path : test_files) {
        if (std::filesystem::exists(file_path)) {
            std::filesystem::remove(file_path);
        }
    }
    
    manager->stop();
}

TEST_F(PipelineIntegrationTest, PipelineBuilderTest) {
    // 使用构建器创建流水线
    auto builder = PipelineBuilder(config_)
        .setWorkers(1, 1, 2)
        .setQueueSizes(5, 10, 15)
        .enableCaching(true)
        .enablePriorityScheduling(true)
        .autoAdjust();
    
    auto manager = builder.build();
    ASSERT_NE(manager, nullptr) << "构建器应该创建有效的管理器";
    
    // 测试流水线功能
    ASSERT_TRUE(manager->start());
    
    bool submitted = manager->processFile(test_file_path_);
    EXPECT_TRUE(submitted);
    
    bool completed = manager->waitForCompletion(3000);
    EXPECT_TRUE(completed);
    
    manager->stop();
}

TEST_F(PipelineIntegrationTest, ResultCallbackTest) {
    auto manager = std::make_unique<PipelineManager>(config_, pipeline_config_);
    
    // 设置结果回调
    std::vector<std::shared_ptr<FunctionAnalysisResult>> callback_results;
    std::mutex callback_mutex;
    
    manager->setResultCallback([&](std::shared_ptr<FunctionAnalysisResult> result) {
        std::lock_guard<std::mutex> lock(callback_mutex);
        callback_results.push_back(result);
    });
    
    ASSERT_TRUE(manager->start());
    
    bool submitted = manager->processFile(test_file_path_);
    EXPECT_TRUE(submitted);
    
    bool completed = manager->waitForCompletion(5000);
    EXPECT_TRUE(completed);
    
    // 验证回调结果
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        EXPECT_GT(callback_results.size(), 0) << "应该通过回调收到结果";
        
        for (const auto& result : callback_results) {
            EXPECT_FALSE(result->function_name.empty()) << "函数名不应为空";
            EXPECT_FALSE(result->file_path.empty()) << "文件路径不应为空";
        }
    }
    
    manager->stop();
}

TEST_F(PipelineIntegrationTest, RealTimeStatsTest) {
    auto manager = std::make_unique<PipelineManager>(config_, pipeline_config_);
    ASSERT_TRUE(manager->start());
    
    // 获取初始统计信息
    std::string initial_stats = manager->getRealTimeStats();
    EXPECT_FALSE(initial_stats.empty()) << "统计信息不应为空";
    EXPECT_NE(initial_stats.find("流水线实时统计"), std::string::npos) << "应包含统计标题";
    
    // 提交文件后再次获取统计
    manager->processFile(test_file_path_);
    
    std::string updated_stats = manager->getRealTimeStats();
    EXPECT_FALSE(updated_stats.empty()) << "更新后的统计信息不应为空";
    
    manager->waitForCompletion(3000);
    manager->stop();
}

TEST_F(PipelineIntegrationTest, ErrorHandlingTest) {
    auto manager = std::make_unique<PipelineManager>(config_, pipeline_config_);
    ASSERT_TRUE(manager->start());
    
    // 尝试处理不存在的文件
    bool submitted = manager->processFile("non_existent_file.cpp");
    // 文件不存在时，提交可能成功（因为检查发生在处理阶段）
    
    // 等待处理（可能会失败）
    manager->waitForCompletion(2000);
    
    // 检查是否有错误信息
    std::string stats = manager->getRealTimeStats();
    // 统计信息中可能包含错误信息
    
    manager->stop();
} 