# PKE 实验 Docker 运行和调试指南

## 前置要求
- 已安装 Docker Desktop
- Docker Desktop 正在运行

## 快速开始

### 方法 1: 使用 PowerShell 脚本（推荐）

在 `riscv-pke` 目录下运行：

```powershell
# 1. 编译并运行（最常用）
.\run_and_debug.ps1 run

# 2. 仅编译
.\run_and_debug.ps1 build

# 3. 编译并显示调试信息
.\run_and_debug.ps1 debug

# 4. 生成反汇编文件（用于分析代码）
.\run_and_debug.ps1 objdump

# 5. 进入容器 Shell（用于手动调试）
.\run_and_debug.ps1 shell

# 6. 清理编译产物
.\run_and_debug.ps1 clean
```

### 方法 2: 直接使用 Docker 命令

```powershell
# 一次性运行（运行完自动删除容器）
docker run --rm -v "${PWD}:/workspace" -w /workspace tjr9098/amd64_pke_mirrors:1.0 bash -c "make clean && make && make run"

# 进入交互式环境
docker run -it --rm -v "${PWD}:/workspace" -w /workspace tjr9098/amd64_pke_mirrors:1.0 /bin/bash
```

### 方法 3: 在容器内使用测试脚本

```powershell
# 1. 进入容器
.\run_and_debug.ps1 shell

# 2. 在容器内运行
chmod +x test.sh
./test.sh
```

## 调试技巧

### 1. 查看编译产物

```bash
# 查看生成的文件
ls -lh obj/

# 查看 ELF 头信息
riscv64-unknown-elf-readelf -h obj/riscv-pke
riscv64-unknown-elf-readelf -h obj/app_print_backtrace

# 查看程序段
riscv64-unknown-elf-readelf -l obj/riscv-pke
```

### 2. 反汇编分析

```bash
# 生成反汇编文件
make objdump

# 查看内核反汇编
cat obj/kernel_dump | less

# 查看应用反汇编
cat obj/user_dump | less

# 查找特定函数
riscv64-unknown-elf-objdump -d obj/app_print_backtrace | grep -A 20 "f1>"
```

### 3. 查看符号表

```bash
# 查看所有符号
riscv64-unknown-elf-nm obj/app_print_backtrace

# 查看函数符号
riscv64-unknown-elf-nm obj/app_print_backtrace | grep " T "

# 查看符号详细信息
riscv64-unknown-elf-readelf -s obj/app_print_backtrace
```

### 4. 逐步调试

在容器内：

```bash
# 1. 清理并编译
make clean && make

# 2. 查看当前分支
git branch --show-current

# 3. 查看提交历史
git log --oneline -5

# 4. 运行程序
make run

# 5. 如果出错，查看详细的编译信息
make clean
make V=1  # 显示详细编译命令
```

### 5. 测试不同的实验

```bash
# 切换到不同的实验分支
git checkout lab1_1_syscall
make clean && make run

git checkout lab1_2_exception  
make clean && make run

git checkout lab1_3_irq
make clean && make run

git checkout lab1_challenge1_backtrace
make clean && make run
```

## 常见问题

### 1. Docker 连接失败
**错误**: `failed to connect to the docker API`
**解决**: 启动 Docker Desktop

### 2. 编译错误
**错误**: 找不到交叉编译工具链
**解决**: 使用提供的 Docker 镜像，里面已经包含了所有工具

### 3. 代码修改后没有效果
**解决**: 
```bash
make clean  # 清理旧的编译产物
make        # 重新编译
make run    # 运行
```

### 4. 查看详细的运行日志
```bash
# 使用调试模式运行
.\run_and_debug.ps1 debug
```

## 实验流程示例

### Lab1_1 系统调用

```powershell
# 1. 确保在 lab1_1 分支
git checkout lab1_1_syscall

# 2. 运行实验
.\run_and_debug.ps1 run

# 3. 应该看到 "Hello world!" 输出
```

### Lab1_Challenge1 调用栈回溯

```powershell
# 1. 切换到 challenge1 分支
git checkout lab1_challenge1_backtrace

# 2. 调试模式运行
.\run_and_debug.ps1 debug

# 3. 应该看到函数调用栈：f8, f7, f6, f5, f4, f3, f2

# 4. 查看反汇编分析栈帧
.\run_and_debug.ps1 objdump
# 然后在容器内查看
.\run_and_debug.ps1 shell
cat obj/user_dump | grep -A 30 "main>"
```

## 性能提示

- 使用 `--rm` 选项可以在容器退出后自动删除，避免占用空间
- 持久化容器（不使用 `--rm`）可以加快后续运行速度
- 编译产物在宿主机的 `obj/` 目录，可以直接查看

## 下一步

完成当前实验后：

```bash
# 提交代码
git commit -a -m "完成 Lab1_X"

# 切换到下一个实验
git checkout lab1_X_xxx
git merge <previous_branch> -m "合并之前的修改"

# 继续实验
.\run_and_debug.ps1 run
```
