# DLogCover
A lightweight AST-based tool for statically analyzing log coverage in C++ projects.

## 简介
DLogCover 是一个基于抽象语法树（AST）的轻量级工具，用于静态分析C++项目中的日志覆盖情况。它可以帮助开发人员评估代码中的日志覆盖率，识别没有适当日志记录的代码区域，从而提高代码的可维护性和问题定位能力。

## 主要功能
- 分析函数级日志覆盖率
- 分析分支级日志覆盖率
- 分析异常处理级日志覆盖率
- 识别关键路径的日志覆盖情况
- 生成日志覆盖率报告
- 提供改进建议

## 项目架构
DLogCover系统的整体结构采用分层架构，主要分为以下几个层次：

1. **命令行接口层**：负责处理用户输入的命令行参数，提供友好的交互界面。
2. **配置管理层**：负责读取和管理配置文件，处理扫描路径、排除规则等配置。
3. **源文件管理层**：负责收集需要分析的源文件，处理文件过滤和路径解析。
4. **AST分析层**：基于Clang/LLVM工具链，对C++源代码进行抽象语法树分析。
5. **日志识别层**：识别代码中的日志函数调用，包括Qt日志函数和自定义日志函数。
6. **覆盖率计算层**：计算各种覆盖率指标，如函数级覆盖率、分支覆盖率等。
7. **报告生成层**：将分析结果整理成易于理解的报告，提供总体覆盖率和未覆盖路径列表等信息。

## 技术栈
- 开发语言：Modern C++ (C++17)
- 静态分析：Clang/LLVM LibTooling
- 配置文件格式：JSON
- 命令行解析：C++17 标准库
- 文件操作：C++17 std::filesystem
- 单元测试：GoogleTest

## 构建与安装

### 依赖项
- CMake 3.10+
- GCC 7+ 或 Clang 5+（支持C++17）
- LLVM/Clang 开发库 (libclang-dev)
- nlohmann_json3-dev (JSON库)
- GoogleTest (可选，用于构建测试)

#### Ubuntu/Debian系统安装依赖
```bash
sudo apt update
sudo apt install build-essential cmake libclang-dev nlohmann-json3-dev
# 如果需要构建测试
sudo apt install libgtest-dev
```

#### CentOS/RHEL系统安装依赖
```bash
sudo yum install gcc-c++ cmake llvm-devel clang-devel
# 如果需要构建测试
sudo yum install gtest-devel
```

### 使用构建脚本
项目提供了一个便捷的构建脚本，可以轻松构建、测试和安装：

```bash
# 默认构建Debug版本
./build.sh

# 构建Release版本
./build.sh --release

# 构建并运行测试
./build.sh --test

# 安装
sudo ./build.sh --release --install

# 清理构建目录
./build.sh --clean

# 显示帮助信息
./build.sh --help
```

### 手动构建步骤
```bash
# 克隆仓库
git clone https://github.com/yourusername/dlogcover.git
cd dlogcover

# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 构建
make

# 运行测试 (可选)
make test

# 安装
sudo make install
```

## 使用说明

### 命令行参数
```
dlogcover [选项]

选项:
  -h, --help                 显示帮助信息
  -d, --directory <path>     指定扫描目录 (默认: ./)
  -o, --output <path>        指定输出报告路径 (默认: ./dlogcover_report_<timestamp>.txt)
  -c, --config <path>        指定配置文件路径 (默认: ./dlogcover.json)
  -e, --exclude <pattern>    排除符合模式的文件或目录 (可多次使用)
  -l, --log-level <level>    指定最低日志级别进行过滤
  -f, --format <format>      指定报告格式 (text, json)
```

### 示例用法
```bash
# 使用默认配置分析当前目录
dlogcover

# 指定目录和输出路径
dlogcover -d /path/to/source -o /path/to/output.txt

# 使用自定义配置文件
dlogcover -c /path/to/config.json

# 排除特定目录
dlogcover -e "build/*" -e "test/*"
```

### 配置文件
配置文件采用JSON格式，示例如下：
```json
{
  "scan": {
    "directories": ["./"],
    "excludes": ["build/", "test/"],
    "file_types": [".cpp", ".cc", ".cxx", ".h", ".hpp"]
  },
  "log_functions": {
    "qt": {
      "enabled": true,
      "functions": ["qDebug", "qInfo", "qWarning", "qCritical", "qFatal"],
      "category_functions": ["qCDebug", "qCInfo", "qCWarning", "qCCritical"]
    },
    "custom": {
      "enabled": true,
      "functions": {
        "debug": ["fmDebug"],
        "info": ["fmInfo"],
        "warning": ["fmWarning"],
        "critical": ["fmCritical"]
      }
    }
  },
  "analysis": {
    "function_coverage": true,
    "branch_coverage": true,
    "exception_coverage": true,
    "key_path_coverage": true
  },
  "report": {
    "format": "text",
    "timestamp_format": "YYYYMMDD_HHMMSS"
  }
}
```

## 测试框架
项目使用分层的测试策略，确保代码质量和功能正确性：

### 单元测试
位于 `tests/unit/` 目录，包含以下测试模块：
- source_manager_test.cpp - 源文件管理模块测试
- log_identifier_test.cpp - 日志识别模块测试
- ast_analyzer_test.cpp - AST分析模块测试
- file_utils_test.cpp - 文件工具模块测试
- command_line_parser_test.cpp - 命令行解析模块测试
- reporter_test.cpp - 报告生成模块测试
- coverage_calculator_test.cpp - 覆盖率计算模块测试
- config_manager_test.cpp - 配置管理模块测试

### 集成测试
位于 `tests/integration/` 目录，测试模块间的交互和完整功能流程。

### 运行测试
```bash
# 使用构建脚本运行所有测试
./build.sh --test

# 或者手动运行测试
cd build
make test

# 生成测试覆盖率报告
make build_test_coverage
```

### 测试覆盖率要求
- 单元测试覆盖率要求：>80%
- 关键模块覆盖率要求：>90%
- 集成测试场景覆盖率：>85%

## 开发指南

### 代码风格
项目使用Modern C++ (C++17)编写，遵循Google C++代码风格指南的变体。我们提供了`.clang-format`配置文件来确保代码格式一致性。在提交代码前，请运行：

```bash
clang-format -i src/*.cpp include/dlogcover/*.h
```

### 测试规范
1. 所有新功能必须包含单元测试
2. 修改现有功能时必须更新相应的测试
3. 测试文件命名规则：`*_test.cpp`
4. 每个测试文件应专注于一个模块的功能测试
5. 测试应该是独立的，不依赖于其他测试的执行顺序
6. 测试应该包含正常情况和异常情况的验证
7. 使用 GoogleTest 的高级特性（如参数化测试）提高测试效率

### 编辑器配置
我们提供了VS Code的配置文件，位于`.vscode/`目录下，包括：
- settings.json - 编辑器设置
- launch.json - 调试配置
- tasks.json - 构建任务

### 日志规范
- 所有函数入口应添加调试级别日志
- 重要逻辑分支应添加适当级别日志
- 异常处理应添加警告或错误级别日志
- 日志内容应包含足够上下文信息便于定位问题

## 实现状态
- [x] 项目结构搭建
- [x] 命令行接口设计
- [x] 配置管理模块设计
- [x] 源文件管理模块设计
- [x] AST分析模块设计
- [x] 日志识别模块设计
- [x] 覆盖率计算模块设计
- [x] 报告生成模块设计
- [x] 命令行接口实现
- [x] 配置管理模块实现
- [x] 源文件管理模块实现
- [x] AST分析模块实现
- [x] 日志识别模块实现
- [x] 覆盖率计算模块实现
- [x] 报告生成模块实现
- [x] 单元测试
- [x] 集成测试
- [x] 文档完善

## 开发进度
- 2023-05-15:
  - 项目基础架构已完成
  - 所有模块已设计并实现基础框架
  - 核心接口已定义并已完成单元测试编写
  - 基本构建过程已验证，可以通过编译和单元测试
  - AST分析、日志识别、覆盖率计算等核心功能需要进一步实现
- 2023-10-20:
  - 项目全部功能已实现完成
  - AST分析、日志识别、覆盖率计算等核心功能已完成
  - 所有单元测试和集成测试已完成并通过
  - 性能优化已完成，满足性能要求
  - 文档已更新完善

## 下一步计划
1. 优化AST分析性能
2. 添加更多自定义日志函数支持
3. 改进报告可视化效果
4. 提供IDE插件集成
5. 扩展对更多语言特性的支持
6. 国际化支持

## 项目文档
- [产品需求文档](docs/产品需求文档.md) - 描述系统需求和用户需求
- [架构设计文档](docs/架构设计文档.md) - 描述系统整体架构和组件关系
- [概要设计文档](docs/概要设计文档.md) - 描述系统主要模块和功能设计
- [详细设计文档](docs/详细设计文档.md) - 描述系统详细设计和实现方案，包含实现计划、测试策略、部署说明等完整内容

## 贡献指南
欢迎提交问题报告和功能请求。如果您想贡献代码，请先fork本仓库，然后创建一个新的分支，提交您的更改，并发送pull请求。

## 许可证
本项目使用 [GNU通用公共许可证第3版(GPLv3)](LICENSE) 许可证
