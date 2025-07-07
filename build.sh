#!/bin/bash
# DLogCover构建脚本

# 错误时退出
set -e

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -h, --help                 显示帮助信息"
    echo "  -c, --clean                清理构建目录"
    echo "  -d, --debug                构建Debug版本(默认)"
    echo "  -r, --release              构建Release版本"
    echo "  -t, --test                 构建并运行测试"
    echo "  -i, --install              安装到系统"
    echo "  -f, --full-process         执行完整流程：编译 -> 测试 -> 覆盖率统计"
    echo "  --package-deb              构建Debian软件包"
    echo "  --prefix=<path>            安装路径前缀"
    echo ""
    echo "环境要求:"
    echo "  - Go 1.15+ (用于Go语言分析器)"
    echo "  - CMake 3.10+"
    echo "  - Clang/LLVM开发库"
    echo "  - nlohmann_json库"
    echo "  - GoogleTest库"
}

# 检查许可证一致性
check_license_consistency() {
    echo "检查许可证一致性..."
    
    if [ -f "tools/check-license-consistency.sh" ]; then
        if ! ./tools/check-license-consistency.sh; then
            echo "错误: 许可证信息不一致"
            echo "请修复许可证信息后重新构建"
            exit 1
        fi
    else
        echo "⚠️  许可证一致性检查脚本不存在，跳过检查"
    fi
}

# 检查Go环境
check_go_environment() {
    echo "检查Go环境..."
    
    # 检查Go是否安装
    if ! command -v go &> /dev/null; then
        echo "错误: Go未安装"
        echo "请安装Go 1.19或更高版本："
        echo "  Ubuntu/Debian: sudo apt install golang-go"
        echo "  CentOS/RHEL: sudo yum install golang"
        echo "  或从官网下载: https://golang.org/dl/"
        exit 1
    fi
    
    # 检查Go版本
    go_version=$(go version | awk '{print $3}' | sed 's/go//')
    required_version="1.15"
    
    if ! printf '%s\n%s\n' "$required_version" "$go_version" | sort -V -C; then
        echo "错误: Go版本过低"
        echo "当前版本: $go_version"
        echo "要求版本: $required_version 或更高"
        echo "请升级Go版本"
        exit 1
    fi
    
    echo "✅ Go环境检查通过 (版本: $go_version)"
}

# 编译Go分析器
build_go_analyzer() {
    echo "编译Go分析器..."
    
    # 检查Go源文件是否存在
    GO_SOURCE_DIR="tools/go-analyzer"
    GO_SOURCE_FILE="$GO_SOURCE_DIR/main.go"
    GO_MOD_FILE="$GO_SOURCE_DIR/go.mod"
    
    if [ ! -f "$GO_SOURCE_FILE" ]; then
        echo "错误: Go源文件不存在: $GO_SOURCE_FILE"
        exit 1
    fi
    
    if [ ! -f "$GO_MOD_FILE" ]; then
        echo "错误: Go模块文件不存在: $GO_MOD_FILE"
        exit 1
    fi
    
    # 确保输出目录存在
    mkdir -p "build/bin"
    
    # 进入Go源码目录
    cd "$GO_SOURCE_DIR"
    
    # 确保Go模块依赖
    echo "检查Go模块依赖..."
    if ! go mod tidy; then
        echo "错误: Go模块依赖处理失败"
        cd - > /dev/null
        exit 1
    fi
    
    # 编译Go程序到build/bin目录
    OUTPUT_BINARY="../../build/bin/dlogcover-go-analyzer"
    echo "编译Go分析器到: build/bin/dlogcover-go-analyzer"
    
    if ! go build -o "$OUTPUT_BINARY" main.go; then
        echo "错误: Go程序编译失败"
        cd - > /dev/null
        exit 1
    fi
    
    # 检查编译结果
    if [ ! -f "$OUTPUT_BINARY" ]; then
        echo "错误: Go编译完成但未找到输出文件"
        cd - > /dev/null
        exit 1
    fi
    
    # 使二进制文件可执行
    chmod +x "$OUTPUT_BINARY"
    
    # 返回原目录
    cd - > /dev/null
    
    echo "✅ Go分析器编译完成: build/bin/dlogcover-go-analyzer"
}

# 清理Go编译产物
clean_go_artifacts() {
    echo "清理Go编译产物..."
    
    # 清理build/bin目录的Go二进制文件
    GO_BINARY="build/bin/dlogcover-go-analyzer"
    if [ -f "$GO_BINARY" ]; then
        rm -f "$GO_BINARY"
        echo "已删除: $GO_BINARY"
    fi
    
    # 清理旧位置的二进制文件（向后兼容）
    OLD_GO_BINARY="tools/go-analyzer/dlogcover-go-analyzer"
    if [ -f "$OLD_GO_BINARY" ]; then
        rm -f "$OLD_GO_BINARY"
        echo "已删除: $OLD_GO_BINARY"
    fi
    
    # 清理bin目录的二进制文件（向后兼容）
    OLD_BIN_BINARY="bin/dlogcover-go-analyzer"
    if [ -f "$OLD_BIN_BINARY" ]; then
        rm -f "$OLD_BIN_BINARY"
        echo "已删除: $OLD_BIN_BINARY"
    fi
    
    # 清理可能的其他位置的二进制文件
    if [ -f "dlogcover-go-analyzer" ]; then
        rm -f "dlogcover-go-analyzer"
        echo "已删除: dlogcover-go-analyzer"
    fi
}

# 默认值
BUILD_TYPE="Debug"
BUILD_TESTS=0
CLEAN=0
INSTALL=0
FULL_PROCESS=0
PACKAGE_DEB=0
INSTALL_PREFIX="/usr/local"

# 保存原始参数用于后续判断
ORIGINAL_ARGS=("$@")

# 解析参数
for arg in "$@"; do
    case $arg in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=1
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            ;;
        -t|--test)
            BUILD_TESTS=1
            ;;
        -i|--install)
            INSTALL=1
            ;;
        -f|--full-process)
            FULL_PROCESS=1
            BUILD_TESTS=1  # 完整流程包含测试
            CLEAN=1        # 完整流程强制清理，确保覆盖率数据一致性
            ;;
        --package-deb)
            PACKAGE_DEB=1
            ;;
        --prefix=*)
            INSTALL_PREFIX="${arg#*=}"
            ;;
        *)
            echo "未知选项: $arg"
            show_help
            exit 1
            ;;
    esac
done

# 创建构建目录
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

# 清理构建
if [ $CLEAN -eq 1 ]; then
    echo "清理构建目录..."
    rm -rf "$BUILD_DIR"/*
    # 清理Go编译产物
    clean_go_artifacts
    # 对于完整流程，额外清理可能的覆盖率文件
    if [ $FULL_PROCESS -eq 1 ]; then
        echo "清理覆盖率数据文件..."
        find . -name "*.gcda" -delete 2>/dev/null || true
        find . -name "*.gcno" -delete 2>/dev/null || true
        rm -f coverage.info coverage.filtered.info 2>/dev/null || true
        rm -rf coverage_report 2>/dev/null || true
    fi
fi

# 判断是否需要检查和编译Go环境
# 纯清理模式（只有-c选项且没有其他构建选项）时不需要Go环境
need_go_build=1
only_clean=0

# 检查是否只有清理选项
if [ ${#ORIGINAL_ARGS[@]} -eq 1 ] && [[ "${ORIGINAL_ARGS[0]}" == "-c" || "${ORIGINAL_ARGS[0]}" == "--clean" ]]; then
    only_clean=1
    need_go_build=0
fi

if [ $need_go_build -eq 1 ]; then
    check_license_consistency
    check_go_environment
    build_go_analyzer
fi

# 如果只是清理模式，跳过后续构建步骤
if [ $only_clean -eq 1 ]; then
    echo "清理完成!"
    exit 0
fi

# 进入构建目录
cd "$BUILD_DIR"

# 配置项目
echo "配置项目 (${BUILD_TYPE})..."
if [ $BUILD_TESTS -eq 1 ]; then
    # 对于测试和完整流程，启用覆盖率
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
else
    # 只构建项目，不构建测试
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTS=OFF -DENABLE_COVERAGE=OFF -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
fi

# 如果是完整流程，使用改进的覆盖率生成流程
if [ $FULL_PROCESS -eq 1 ]; then
    echo "执行完整流程：编译 -> 测试 -> 覆盖率统计..."
    
    # 检查必要工具
    if ! command -v lcov &> /dev/null; then
        echo "错误: lcov 未安装，请使用 'sudo apt install lcov' 安装"
        exit 1
    fi
    if ! command -v genhtml &> /dev/null; then
        echo "错误: genhtml 未安装，它是lcov包的一部分"
        exit 1
    fi

    # 步骤1: 完全重新编译项目和测试
    echo "步骤1: 重新编译项目和测试..."
    cmake --build . -- -j$(nproc)
    
    # 步骤2: 清零覆盖率计数器
    echo "步骤2: 清零覆盖率计数器..."
    lcov --directory . --zerocounters
    
    # 步骤3: 运行测试
    echo "步骤3: 运行测试..."
    if ! ctest --verbose; then
        echo "错误: 测试执行失败，无法生成覆盖率报告"
        exit 1
    fi
    
    # 步骤4: 检查覆盖率数据文件完整性
    echo "步骤4: 检查覆盖率数据文件完整性..."
    gcda_files=$(find . -name "*.gcda" | wc -l)
    gcno_files=$(find . -name "*.gcno" | wc -l)
    echo "找到 $gcda_files 个 .gcda 文件和 $gcno_files 个 .gcno 文件"
    
    # 检查是否有不匹配的文件
    missing_gcno=0
    find . -name "*.gcda" | while read gcda; do
        gcno="${gcda%.gcda}.gcno"
        if [ ! -f "$gcno" ]; then
            echo "警告: 缺失对应的 .gcno 文件: $gcno"
            missing_gcno=$((missing_gcno + 1))
        fi
    done
    
    # 步骤5: 收集覆盖率数据
    echo "步骤5: 收集覆盖率数据..."
    if ! lcov --directory . --capture --output-file coverage.info; then
        echo "错误: 覆盖率数据收集失败"
        echo "可能的原因:"
        echo "1. .gcno 和 .gcda 文件不匹配"
        echo "2. 编译时覆盖率标志未正确应用"
        echo "3. 测试未正确执行"
        echo ""
        echo "尝试解决方案:"
        echo "1. 完全清理并重新构建: ./build.sh -c -f"
        echo "2. 检查编译器版本和lcov版本兼容性"
        exit 1
    fi
    
    # 步骤6: 过滤覆盖率数据
    echo "步骤6: 过滤覆盖率数据..."
    lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.filtered.info
    
    # 步骤7: 生成HTML报告
    echo "步骤7: 生成HTML报告..."
    genhtml coverage.filtered.info --output-directory coverage_report
    
    # 步骤8: 显示覆盖率摘要
    echo "步骤8: 显示覆盖率摘要..."
    lcov --summary coverage.filtered.info
    
    # 步骤9: 检查覆盖率要求
    echo "步骤9: 检查覆盖率要求..."
    cmake -DCMAKE_SOURCE_DIR=.. -DCMAKE_BINARY_DIR=. -P ../cmake/CheckCoverage.cmake

    echo ""
    echo "✅ 完整流程执行完成！"
    echo "📊 项目编译成功"
    echo "🧪 单元测试执行完成"
    echo "📈 覆盖率报告生成完成"
    echo "📁 覆盖率报告位置: ${PWD}/coverage_report/index.html"
    echo ""
    echo "💡 提示: 可以使用浏览器打开覆盖率报告查看详细信息"
else
    # 构建项目
    echo "构建项目..."
    if [ $BUILD_TESTS -eq 1 ]; then
        # 构建项目和测试
        cmake --build . -- -j$(nproc)
    else
        # 只构建主项目
        cmake --build . --target dlogcover -- -j$(nproc)
    fi

    # 运行测试
    if [ $BUILD_TESTS -eq 1 ]; then
        echo "运行测试..."
        ctest --verbose
    fi
fi

# 安装
if [ $INSTALL -eq 1 ]; then
    echo "安装到 ${INSTALL_PREFIX}..."
    cmake --install .
fi

# 如果不是完整流程，显示常规完成消息
if [ $FULL_PROCESS -eq 0 ]; then
    echo "构建完成!"
fi

# 如果需要构建Debian软件包
if [ $PACKAGE_DEB -eq 1 ]; then
    echo ""
    echo "开始构建Debian软件包..."
    
    # 确保已安装dpkg-dev
    if ! command -v dpkg-buildpackage &> /dev/null; then
        echo "错误: dpkg-buildpackage 未找到。"
        echo "请安装打包工具: sudo apt-get install dpkg-dev debhelper"
        exit 1
    fi
    
    # -us: 不对源码包签名
    # -uc: 不对 .changes 文件签名
    # -b: 只构建二进制包
    # --no-git: 强制dpkg-buildpackage不与git交互，避免因git状态导致的不可预测问题
    # dpkg-buildpackage会自动调用debian/rules,后者会负责编译
    if dpkg-buildpackage -us -uc -b --no-git; then
        echo "✅ Debian软件包构建成功！"
        echo "你可以在上一级目录找到 .deb 文件。"
    else
        echo "❌ Debian软件包构建失败。"
        exit 1
    fi
    # 打包完成后直接退出，避免执行后续的cd ..
    exit 0
fi

# 如果测试通过，并且启用了覆盖率
if [ $BUILD_TESTS -eq 1 ] && [ $ENABLE_COVERAGE -eq 1 ]; then
    # ... existing code ...
fi

cd ..
