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
fi

# 进入构建目录
cd "$BUILD_DIR"

# 配置项目
echo "配置项目 (${BUILD_TYPE})..."
if [ $BUILD_TESTS -eq 1 ]; then
    # 对于测试和完整流程，启用覆盖率
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
else
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
fi

# 如果是完整流程，使用build_test_coverage目标
if [ $FULL_PROCESS -eq 1 ]; then
    echo "执行完整流程：编译 -> 测试 -> 覆盖率统计..."
    # 检查lcov是否安装
    if ! command -v lcov &> /dev/null
    then
        echo "错误: lcov 未安装，请使用 'sudo apt install lcov' 安装"
        exit 1
    fi
    # 检查genhtml是否安装
    if ! command -v genhtml &> /dev/null
    then
        echo "错误: genhtml 未安装，它是lcov包的一部分"
        exit 1
    fi

    # 使用CMake的build_test_coverage目标
    cmake --build . --target build_test_coverage

    echo "完整流程执行完成！"
    echo "1. 项目编译成功"
    echo "2. 单元测试执行完成"
    echo "3. 覆盖率报告生成完成"
    echo "覆盖率报告位置: ${PWD}/coverage_report/index.html"
else
# 构建项目
echo "构建项目..."
cmake --build . -- -j$(nproc)

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
