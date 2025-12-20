#!/bin/bash
# PKE 容器内快速测试脚本
# 在容器内使用：./test.sh

set -e

echo "====== PKE 快速测试 ======"
echo "清理旧的编译产物..."
make clean

echo ""
echo "编译内核和应用程序..."
make

echo ""
echo "====== 内核信息 ======"
riscv64-unknown-elf-readelf -h obj/riscv-pke | grep -E "Entry|Machine"

echo ""
echo "====== 应用程序信息 ======"
riscv64-unknown-elf-readelf -h obj/app_* | grep -E "Entry|Machine"

echo ""
echo "====== 运行实验 ======"
echo "当前分支: $(git branch --show-current)"
echo "开始执行..."
echo "----------------------------------------"
make run
echo "----------------------------------------"
echo ""
echo "====== 测试完成 ======"
