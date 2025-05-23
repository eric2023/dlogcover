<!-- Generated by SpecStory -->

# Cursor Rules Generation (2025-05-14 08:57:31)

_**User**_

/Generate Cursor Rules 

---

_**Assistant**_



---

Let me generate Cursor Rules for your project. First, I'll examine the project structure to create meaningful rules.

---

<details>
            <summary>Listed current directory • **5** results</summary>
        
| Name |
|-------|
| 📁 `.git` |
| 📁 `docs` |
| 📁 `.cursor` |
| 📄 `LICENSE` |
| 📄 `README.md` |

</details>

---

Let me check the README.md file to understand the project:

---

Read file: README.md

---

Now let me check the codebase structure further:

---

<details>
            <summary>Listed directory docs • **1** results</summary>
        
| Name |
|-------|
| 📄 `PRD.md` |

</details>

---

Let me read the PRD.md file to understand the project requirements:

---

Read file: docs/PRD.md

---

Now let me examine the source code structure:

---

Could not resolve URI: Error: 无法解析不存在的文件 '/data/workspace/git/dlogcover/src'

---

Let me check if there's any other top-level directories:

---

```bash
find . -maxdepth 2 -type d -not -path "*/\.*" | sort
```

---

Let me check if there's a `.cursor` directory that might contain existing rules:

---

<details>
            <summary>Listed directory .cursor • **1** results</summary>
        
| Name |
|-------|
| 📁 `rules` |

</details>

---

Let's check if there are any existing rules:

---

<details>
            <summary>Listed directory .cursor/rules • **8** results</summary>
        
| Name |
|-------|
| 📄 `03-development-workflow.mdc` |
| 📄 `02-development-environment.mdc` |
| 📄 `01-project-overview.mdc` |
| 📄 `03-development-guide.mdc` |
| 📄 `03-workflow.mdc` |
| 📄 `02-project-structure.mdc` |
| 📄 `00-project-overview.mdc` |
| 📄 `01-development-guidelines.mdc` |

</details>

---

Let me look at the content of one of the existing rules to understand the format:

---

Read file: .cursor/rules/01-project-overview.mdc

---

Let me check another rule file:

---

Read file: .cursor/rules/00-project-overview.mdc

---

Based on the inspection, I need to:
1. Generate proper Cursor Rules for this project
2. The project appears to be a C++ tool for analyzing log coverage in C++ projects
3. I should create or update rules in the `.cursor/rules` directory

Let me create a set of rules that will help with navigating and understanding this codebase:

---

```diff
- 
+ # DLogCover 项目概述
+ 
+ DLogCover 是一个轻量级的基于 AST 的静态分析工具，用于分析 C++ 项目中的日志覆盖率情况。该工具通过静态代码分析来评估代码中的日志覆盖情况，帮助开发人员识别可能缺少必要日志记录的关键代码路径。
+ 
+ 项目背景：在 C++ 项目开发中，日志对于问题定位和系统监控至关重要。然而，目前缺乏有效的工具来评估代码中的日志覆盖情况，这可能导致关键代码路径缺少必要的日志记录。
+ 
+ 项目价值：
+ - 提高代码质量和可维护性
+ - 降低生产环境问题定位难度
+ - 减少运维成本
+ - 提升开发团队效率
+ 
+ 目标用户：
+ - C++ 开发工程师
+ - 质量保证工程师
  
```

---

```diff
- 
+ # DLogCover 项目结构
+ 
+ ## 目录结构
+ - `docs/` - 项目文档目录
+   - `PRD.md` - 产品需求文档，详细描述了项目的功能需求和非功能需求
+ - 待创建的目录:
+   - `src/` - 源代码目录
+     - `core/` - 核心分析引擎
+     - `parser/` - C++ 代码解析模块
+     - `analyzer/` - 日志覆盖率分析模块
+     - `reporter/` - 报告生成模块
+     - `utils/` - 工具类和辅助函数
+   - `include/` - 头文件目录
+   - `tests/` - 测试用例目录
+   - `examples/` - 示例代码目录
+   - `build/` - 构建输出目录
+ 
+ ## 主要模块
+ 1. **命令行接口** - 处理用户输入参数和配置
+ 2. **代码解析器** - 解析 C++ 源代码并构建 AST
+ 3. **日志识别器** - 识别代码中的日志函数调用
+ 4. **覆盖率分析器** - 分析代码路径的日志覆盖情况
+ 5. **报告生成器** - 生成覆盖率报告
+ 
+ 注意：此项目目前处于初始阶段，上述结构可能会随着开发进程调整。
+ 
```

---

```diff
     - test: 测试相关
     - chore: 构建过程或辅助工具的变动
+ 
+ # DLogCover 开发环境配置
+ 
+ ## 构建环境要求
+ - 现代 C++ 编译器，支持 C++17 标准
+   - GCC 7.0+ 或 Clang 5.0+
+ - CMake 3.10+
+ - LLVM/Clang 库和开发包 (用于 AST 解析)
+ - jsoncpp 库 (用于配置文件解析)
+ 
+ ## 开发工具推荐
+ - IDE：Visual Studio Code、CLion 或 Qt Creator
+ - 版本控制：Git
+ - 代码风格检查：clang-format
+ - 静态分析：clang-tidy
+ - 单元测试：Google Test
+ - 性能测试：Google Benchmark
+ 
+ ## 环境配置步骤
+ 
+ ### Linux 环境 (Ubuntu/Debian)
+ ```bash
+ # 安装编译工具和基础库
+ sudo apt update
+ sudo apt install -y build-essential cmake git
+ 
+ # 安装 LLVM/Clang 开发包
+ sudo apt install -y llvm-dev libclang-dev clang
+ 
+ # 安装 jsoncpp
+ sudo apt install -y libjsoncpp-dev
+ 
+ # 安装开发工具
+ sudo apt install -y clang-format clang-tidy
+ ```
+ 
+ ### Windows 环境
+ - 安装 Visual Studio 2019+ 或 MinGW-w64
+ - 安装 CMake
+ - 安装 LLVM/Clang
+ - 使用 vcpkg 安装依赖库：
+   ```
+   vcpkg install jsoncpp
+   ```
+ 
+ ### macOS 环境
+ ```bash
+ # 使用 Homebrew 安装依赖
+ brew install cmake llvm jsoncpp
+ ```
+ 
+ ## 构建指南
+ ```bash
+ # 克隆仓库
+ git clone <repository-url>
+ cd dlogcover
+ 
+ # 创建并进入构建目录
+ mkdir build && cd build
+ 
+ # 配置项目
+ cmake ..
+ 
+ # 构建项目
+ cmake --build .
+ ```
  
```

---

Cancelled

---

