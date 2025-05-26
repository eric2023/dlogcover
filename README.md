# DLogCover - C++日志覆盖分析工具

DLogCover是一个基于Clang/LLVM的C++日志覆盖分析工具，用于分析C++项目中的日志记录覆盖情况。

## 项目概述

DLogCover通过分析C++源代码的抽象语法树(AST)，识别代码中的函数、分支、循环、异常处理等结构，并检测这些结构中是否包含适当的日志记录。工具支持Qt日志系统和自定义日志函数，能够生成详细的覆盖率报告。

## 最新更新 (2024)

### 头文件重新生成
项目的.h头文件已经根据对应的.cpp实现文件和项目规范重新生成，包括：

#### CLI模块
- `include/dlogcover/cli/config_constants.h` - 配置常量定义
- `include/dlogcover/cli/error_types.h` - 错误类型枚举
- `include/dlogcover/cli/config_validator.h` - 配置验证器
- `include/dlogcover/cli/options.h` - 命令行选项定义

#### 工具模块
- `include/dlogcover/utils/log_utils.h` - 日志工具函数
- `include/dlogcover/utils/file_utils.h` - 文件操作工具
- `include/dlogcover/utils/string_utils.h` - 字符串处理工具

#### 核心模块
- `include/dlogcover/core/ast_analyzer/ast_types.h` - AST分析器类型定义
- `include/dlogcover/core/ast_analyzer/ast_analyzer.h` - AST分析器主要接口
- `include/dlogcover/core/log_identifier/log_types.h` - 日志类型定义
- `include/dlogcover/core/log_identifier/log_identifier.h` - 日志识别器
- `include/dlogcover/core/coverage/coverage_stats.h` - 覆盖率统计信息
- `include/dlogcover/core/coverage/coverage_calculator.h` - 覆盖率计算器

#### 源文件管理
- `include/dlogcover/source_manager/source_manager.h` - 源文件管理器

#### 报告生成
- `include/dlogcover/reporter/reporter_types.h` - 报告类型定义
- `include/dlogcover/reporter/ireporter_strategy.h` - 报告策略接口
- `include/dlogcover/reporter/reporter.h` - 报告生成器

### 头文件特点
- 严格遵循项目代码规范
- 完整的文档注释
- 合理的命名空间组织
- 清晰的依赖关系
- 支持现代C++特性

## 主要功能

### 1. 多层次覆盖分析
- **函数覆盖率**: 分析函数是否包含日志记录
- **分支覆盖率**: 检测if/else、switch等分支结构的日志覆盖
- **循环覆盖率**: 分析for、while等循环结构的日志记录
- **异常处理覆盖率**: 检测try-catch块的日志覆盖情况
- **关键路径覆盖率**: 识别和分析关键执行路径的日志覆盖

### 2. 灵活的日志系统支持
- **Qt日志系统**: 支持qDebug、qInfo、qWarning、qCritical、qFatal等
- **Qt分类日志**: 支持qCDebug、qCInfo等分类日志函数
- **自定义日志**: 通过配置文件支持任意自定义日志函数
- **日志级别过滤**: 支持按日志级别进行过滤分析

### 3. 多格式报告输出
- **文本格式**: 易读的文本报告
- **JSON格式**: 结构化数据，便于集成其他工具
- **详细统计**: 提供文件级和项目级的详细统计信息

### 4. 灵活的配置系统
- **JSON配置文件**: 支持复杂的配置选项
- **命令行参数**: 支持常用参数的命令行覆盖
- **排除模式**: 支持正则表达式排除特定文件或目录

## 技术架构

### 核心组件
1. **AST分析器** (`core/ast_analyzer/`): 基于Clang解析C++源代码
2. **日志识别器** (`core/log_identifier/`): 识别代码中的日志调用
3. **覆盖率计算器** (`core/coverage/`): 计算各种类型的覆盖率
4. **源文件管理器** (`source_manager/`): 管理源文件的收集和处理
5. **报告生成器** (`reporter/`): 生成各种格式的覆盖率报告

### 支持的编译器
- GCC 7.0+
- Clang 6.0+
- 需要C++17支持

## 安装和构建

### 依赖项
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libclang-dev llvm-dev nlohmann-json3-dev

# CentOS/RHEL
sudo yum install gcc-c++ cmake clang-devel llvm-devel nlohmann-json-devel
```

### 构建步骤
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 使用方法

### 基本用法
```bash
# 分析当前目录
./dlogcover

# 分析指定目录
./dlogcover -d /path/to/project

# 使用配置文件
./dlogcover -c config.json

# 生成JSON格式报告
./dlogcover -d /path/to/project -o report.json -f json
```

### 配置文件示例
```json
{
    "version": "1.0",
    "scan": {
        "directories": ["./src", "./include"],
        "excludes": ["build/", "test/", "third_party/"],
        "fileTypes": [".cpp", ".cc", ".cxx", ".h", ".hpp"],
        "isQtProject": true
    },
    "logFunctions": {
        "qt": {
            "enabled": true,
            "functions": ["qDebug", "qInfo", "qWarning", "qCritical", "qFatal"],
            "categoryFunctions": ["qCDebug", "qCInfo", "qCWarning", "qCCritical"]
        },
        "custom": {
            "enabled": true,
            "functions": {
                "debug": ["LOG_DEBUG", "logDebug"],
                "info": ["LOG_INFO", "logInfo"],
                "warning": ["LOG_WARNING", "logWarning"],
                "error": ["LOG_ERROR", "logError"],
                "fatal": ["LOG_FATAL", "logFatal"]
            }
        }
    },
    "analysis": {
        "functionCoverage": true,
        "branchCoverage": true,
        "exceptionCoverage": true,
        "keyPathCoverage": true
    },
    "report": {
        "format": "text",
        "timestampFormat": "YYYYMMDD_HHMMSS"
    }
}
```

## 命令行选项

| 选项 | 描述 | 默认值 |
|------|------|--------|
| `-d, --directory` | 扫描目录路径 | 当前目录 |
| `-o, --output` | 输出文件路径 | 标准输出 |
| `-c, --config` | 配置文件路径 | 无 |
| `-f, --format` | 报告格式 (text/json) | text |
| `-l, --log-level` | 日志级别过滤 | all |
| `-e, --exclude` | 排除模式 | 无 |
| `-h, --help` | 显示帮助信息 | - |
| `-v, --version` | 显示版本信息 | - |

## 报告示例

### 文本格式报告
```
DLogCover 覆盖率报告
====================
项目: MyProject
时间: 2024-01-15 14:30:25

总体统计:
- 总文件数: 25
- 有覆盖的文件数: 20
- 总函数数: 150
- 有日志的函数数: 120
- 函数覆盖率: 80.00%
- 分支覆盖率: 75.50%
- 异常处理覆盖率: 90.00%
- 关键路径覆盖率: 85.00%
- 总体覆盖率: 82.63%

文件详情:
src/main.cpp: 95.00% (19/20 函数有日志)
src/utils.cpp: 70.00% (14/20 函数有日志)
...
```

### JSON格式报告
```json
{
    "projectName": "MyProject",
    "timestamp": "2024-01-15T14:30:25",
    "overallCoverageRatio": 0.8263,
    "totalFiles": 25,
    "coveredFiles": 20,
    "totalFunctions": 150,
    "coveredFunctions": 120,
    "functionCoverageRatio": 0.8000,
    "files": [
        {
            "filePath": "src/main.cpp",
            "functionCoverageRatio": 0.95,
            "functions": [...]
        }
    ]
}
```

## 开发和贡献

### 代码规范
- 遵循Google C++代码规范
- 使用现代C++特性(C++17)
- 完整的单元测试覆盖
- 详细的代码注释

### 测试
```bash
# 运行单元测试
make test

# 运行集成测试
./tests/integration_test.sh
```

### 贡献指南
1. Fork项目
2. 创建特性分支
3. 提交更改
4. 创建Pull Request

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 联系方式

- 项目主页: https://github.com/dlogcover/dlogcover
- 问题反馈: https://github.com/dlogcover/dlogcover/issues
- 邮箱: dlogcover@example.com

## 更新日志

### v1.2.0 (2024-01-15)
- 重新生成所有头文件，确保与实现文件一致
- 改进代码结构和命名空间组织
- 增强错误处理和日志记录
- 优化AST分析性能

### v1.1.0 (2023-12-01)
- 添加关键路径覆盖分析
- 支持Qt分类日志函数
- 改进配置文件验证
- 增加JSON格式报告

### v1.0.0 (2023-10-01)
- 初始版本发布
- 基本的函数和分支覆盖分析
- 支持Qt和自定义日志函数
- 文本格式报告输出
