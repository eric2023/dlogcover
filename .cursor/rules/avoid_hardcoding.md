# DLogCover项目规则：避免硬编码

## 避免硬编码规则

1. **配置数据**：
   - 所有可能变更的配置应存储在配置文件中，代码中通过配置对象读取
   - 避免在代码中硬编码配置值，如文件路径、目录名称、日志级别等

2. **日志函数名**：
   - 日志函数和宏名称应从配置中读取，不应在代码中硬编码
   - 使用`config_.logFunctions`结构获取日志函数配置
   - 在识别日志调用时使用配置中的函数名列表

3. **文件路径和目录**：
   - 避免硬编码文件路径和目录
   - 使用相对路径或从配置中读取的路径
   - 所有默认路径应定义在配置对象中

4. **魔法数字**：
   - 避免在代码中使用无含义的魔法数字
   - 使用命名常量定义有特定含义的数值
   - 配置相关的数值应从配置中读取

5. **关键字列表**：
   - 不应硬编码识别关键字，如错误关键字、日志前缀等
   - 使用配置文件定义关键字列表
   - 使用具名常量定义固定的关键字列表

## 实现示例

### 1. 从配置读取日志函数名：

```cpp
// 错误示例:
if (node->text.find("LOG_") != std::string::npos) {
    nodeInfo->hasLogging = true;
}

// 正确示例:
bool containsLogKeywords(const std::string& text) const {
    // 检查所有配置的日志函数
    if (config_.logFunctions.custom.enabled) {
        for (const auto& [level, funcs] : config_.logFunctions.custom.functions) {
            for (const auto& func : funcs) {
                if (text.find(func) != std::string::npos) {
                    return true;
                }
            }
        }
    }
    return false;
}
```

### 2. 使用命名常量替代魔法数字：

```cpp
// 错误示例:
if (overallStats_.totalFunctions == 0 && overallStats_.totalBranches == 0) {
    LOG_WARNING("警告：未检测到任何代码元素");
}

// 正确示例:
static constexpr int MIN_REQUIRED_FUNCTIONS = 0;
static constexpr int MIN_REQUIRED_BRANCHES = 0;

if (overallStats_.totalFunctions == MIN_REQUIRED_FUNCTIONS &&
    overallStats_.totalBranches == MIN_REQUIRED_BRANCHES) {
    LOG_WARNING("警告：未检测到任何代码元素");
}
```

### 3. 关键字列表使用命名常量：

```cpp
// 错误示例:
if (node->text.find("error") != std::string::npos ||
    node->text.find("fail") != std::string::npos) {
    keyBranches.push_back(node);
}

// 正确示例:
static const std::vector<std::string> ERROR_KEYWORDS = {
    "error", "fail", "exception", "return false", "return -1", "throw"
};

for (const auto& keyword : ERROR_KEYWORDS) {
    if (node->text.find(keyword) != std::string::npos) {
        keyBranches.push_back(node);
        break;
    }
}
```
