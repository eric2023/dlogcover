#!/bin/bash
# DLogCover构建脚本

# 错误时退出
set -e

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -h, --help         显示帮助信息"
    echo "  -c, --clean        清理构建目录"
    echo "  -d, --debug        构建Debug版本(默认)"
    echo "  -r, --release      构建Release版本"
    echo "  -t, --test         构建并运行测试"
    echo "  -i, --install      安装到系统"
    echo "  --prefix=<path>    安装路径前缀"
}

# 默认值
BUILD_TYPE="Debug"
BUILD_TESTS=0
CLEAN=0
INSTALL=0
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
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTS=ON -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
else
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
fi

# 构建项目
echo "构建项目..."
cmake --build . -- -j$(nproc)

# 运行测试
if [ $BUILD_TESTS -eq 1 ]; then
    echo "运行测试..."
    ctest --verbose
fi

# 安装
if [ $INSTALL -eq 1 ]; then
    echo "安装到 ${INSTALL_PREFIX}..."
    cmake --install .
fi

echo "构建完成!"
cd .. 