/**
 * @file language_detector_test.cpp
 * @brief 语言检测器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/language_detector/language_detector.h>

using namespace dlogcover::core::language_detector;

class LanguageDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的设置
    }

    void TearDown() override {
        // 测试后的清理
    }
};

// 测试C++文件检测
TEST_F(LanguageDetectorTest, DetectCppFiles) {
    EXPECT_EQ(LanguageDetector::detectLanguage("test.cpp"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.cxx"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.cc"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.c++"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.C"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.h"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.hpp"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.hxx"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.h++"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.hh"), SourceLanguage::CPP);
}

// 测试Go文件检测
TEST_F(LanguageDetectorTest, DetectGoFiles) {
    EXPECT_EQ(LanguageDetector::detectLanguage("main.go"), SourceLanguage::GO);
    EXPECT_EQ(LanguageDetector::detectLanguage("package.go"), SourceLanguage::GO);
    EXPECT_EQ(LanguageDetector::detectLanguage("/path/to/file.go"), SourceLanguage::GO);
}

// 测试未知文件类型
TEST_F(LanguageDetectorTest, DetectUnknownFiles) {
    EXPECT_EQ(LanguageDetector::detectLanguage("test.txt"), SourceLanguage::UNKNOWN);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.py"), SourceLanguage::UNKNOWN);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.java"), SourceLanguage::UNKNOWN);
    EXPECT_EQ(LanguageDetector::detectLanguage("test"), SourceLanguage::UNKNOWN);
    EXPECT_EQ(LanguageDetector::detectLanguage(""), SourceLanguage::UNKNOWN);
}

// 测试大小写不敏感
TEST_F(LanguageDetectorTest, CaseInsensitive) {
    EXPECT_EQ(LanguageDetector::detectLanguage("test.CPP"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.GO"), SourceLanguage::GO);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.Cpp"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("test.Go"), SourceLanguage::GO);
}

// 测试路径处理
TEST_F(LanguageDetectorTest, PathHandling) {
    EXPECT_EQ(LanguageDetector::detectLanguage("/usr/src/project/main.cpp"), SourceLanguage::CPP);
    EXPECT_EQ(LanguageDetector::detectLanguage("./src/main.go"), SourceLanguage::GO);
    EXPECT_EQ(LanguageDetector::detectLanguage("../include/header.h"), SourceLanguage::CPP);
}

// 测试扩展名检查函数
TEST_F(LanguageDetectorTest, ExtensionCheckers) {
    EXPECT_TRUE(LanguageDetector::hasCppExtension("test.cpp"));
    EXPECT_TRUE(LanguageDetector::hasCppExtension("test.h"));
    EXPECT_FALSE(LanguageDetector::hasCppExtension("test.go"));
    EXPECT_FALSE(LanguageDetector::hasCppExtension("test.txt"));

    EXPECT_TRUE(LanguageDetector::hasGoExtension("test.go"));
    EXPECT_FALSE(LanguageDetector::hasGoExtension("test.cpp"));
    EXPECT_FALSE(LanguageDetector::hasGoExtension("test.h"));
}

// 测试语言名称获取
TEST_F(LanguageDetectorTest, LanguageNames) {
    EXPECT_EQ(LanguageDetector::getLanguageName(SourceLanguage::CPP), "C++");
    EXPECT_EQ(LanguageDetector::getLanguageName(SourceLanguage::GO), "Go");
    EXPECT_EQ(LanguageDetector::getLanguageName(SourceLanguage::UNKNOWN), "Unknown");
} 