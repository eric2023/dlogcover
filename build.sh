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
    echo "  --prefix=<path>            安装路径前缀"
}

# 默认值
BUILD_TYPE="Debug"
BUILD_TESTS=0
CLEAN=0
INSTALL=0
FULL_PROCESS=0
INSTALL_PREFIX="/usr/local"

# 解析参数
for arg in "$@"; do
    case $arg in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -t|--test)
            BUILD_TESTS=1
            shift
            ;;
        -i|--install)
            INSTALL=1
            shift
            ;;
        -f|--full-process)
            FULL_PROCESS=1
            BUILD_TESTS=1  # 完整流程包含测试
            CLEAN=1        # 完整流程强制清理，确保覆盖率数据一致性
            shift
            ;;
        --prefix=*)
            INSTALL_PREFIX="${arg#*=}"
            shift
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
    # 对于完整流程，额外清理可能的覆盖率文件
    if [ $FULL_PROCESS -eq 1 ]; then
        echo "清理覆盖率数据文件..."
        find . -name "*.gcda" -delete 2>/dev/null || true
        find . -name "*.gcno" -delete 2>/dev/null || true
        rm -f coverage.info coverage.filtered.info 2>/dev/null || true
        rm -rf coverage_report 2>/dev/null || true
    fi
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

cd ..
