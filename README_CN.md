# OS2025 PKE 操作系统实验

本仓库是华中科技大学操作系统课程 PKE (Proxy Kernel for Education) 实验的代码实现。

## 实验环境

- **架构**: RISC-V RV64G
- **模拟器**: Spike RISC-V ISA Simulator
- **工具链**: riscv64-unknown-elf-gcc
- **容器**: Docker (tjr9098/amd64_pke_mirrors:1.0)

## 实验进度

### ✅ Lab1: 中断、异常和系统调用

- [x] **Lab1_1**: 系统调用 ([lab1_1_syscall](../../tree/lab1_1_syscall))
  - 实现系统调用机制，使用户程序能够通过 `ecall` 调用内核服务
  
- [x] **Lab1_2**: 异常处理 ([lab1_2_exception](../../tree/lab1_2_exception))
  - 处理非法指令异常，避免系统崩溃
  
- [x] **Lab1_3**: 时钟中断 ([lab1_3_irq](../../tree/lab1_3_irq))
  - 实现定时器中断处理和时钟滴答计数
  
- [x] **Lab1_Challenge1**: 调用栈回溯 ([lab1_challenge1_backtrace](../../tree/lab1_challenge1_backtrace))
  - 实现用户程序调用栈回溯功能
  - 解析 ELF 符号表，将返回地址映射到函数名
  - 支持可变深度的栈回溯
  - 📄 [实验报告](LAB1_REPORT.md)

### 🔜 Lab2: 虚拟内存管理

- [ ] Lab2_1: 页表
- [ ] Lab2_2: 页面分配
- [ ] Lab2_3: 缺页异常

### 🔜 Lab3: 进程管理

- [ ] Lab3_1: Fork
- [ ] Lab3_2: Yield
- [ ] Lab3_3: 调度器

### 🔜 Lab4: 文件系统

- [ ] Lab4_1: 文件操作
- [ ] Lab4_2: 目录操作
- [ ] Lab4_3: 硬链接

### 🔜 Lab5: 设备管理

- [ ] Lab5_1: 轮询
- [ ] Lab5_2: PLIC
- [ ] Lab5_3: 主机设备

## 快速开始

### 使用 Docker

```bash
# 构建项目
docker run --rm -v ${PWD}:/workspace -w /workspace \
  tjr9098/amd64_pke_mirrors:1.0 make

# 运行实验
docker run --rm -v ${PWD}:/workspace -w /workspace \
  tjr9098/amd64_pke_mirrors:1.0 \
  spike obj/riscv-pke obj/app_print_backtrace
```

### 使用自动化脚本 (Windows PowerShell)

```powershell
# 运行实验
.\run_and_debug.ps1 run

# 调试模式
.\run_and_debug.ps1 debug

# 查看反汇编
.\run_and_debug.ps1 objdump app_print_backtrace

# 进入容器 shell
.\run_and_debug.ps1 shell
```

详见 [Docker 使用指南](DOCKER_GUIDE.md)

## 项目结构

```
riscv-pke/
├── kernel/              # 内核代码
│   ├── kernel.c        # 内核主程序
│   ├── strap.c         # S-mode 陷阱处理
│   ├── syscall.c       # 系统调用实现
│   ├── elf.c           # ELF 文件解析
│   ├── process.c       # 进程管理
│   └── machine/        # M-mode 代码
├── user/               # 用户程序
│   ├── app_*.c         # 测试应用
│   ├── user_lib.c      # 用户库
│   └── user.lds        # 链接脚本
├── spike_interface/    # Spike 模拟器接口
└── util/               # 工具函数

obj/                    # 编译输出目录
├── riscv-pke          # 内核可执行文件
└── app_*              # 用户程序可执行文件
```

## 关键技术点

### Lab1_Challenge1 实现亮点

1. **栈帧遍历算法**
   - 正确处理不同大小的栈帧（do_user_call 32字节 vs 普通函数 16字节）
   - 从 trapframe 恢复用户态寄存器状态
   - 实现栈指针有效性检查

2. **ELF 符号表解析**
   - 解析 `.symtab` 和 `.strtab` 段
   - 实现地址到函数名的映射
   - 支持函数地址范围匹配

3. **系统调用机制**
   - 实现 `SYS_user_print_backtrace` 系统调用
   - 正确传递用户态参数到内核
   - 保证内核态和用户态栈的独立性

## 开发工具

- **调试**: 使用 `objdump` 分析汇编代码
- **容器**: Docker 提供统一的编译环境
- **版本控制**: Git 分支管理不同实验阶段

## 上游仓库

- 原始仓库: https://gitee.com/hustos/riscv-pke
- 文档仓库: https://gitee.com/hustos/pke-doc

## 参考资料

- [RISC-V Instruction Set Manual](https://riscv.org/technical/specifications/)
- [xv6-riscv Book](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)
- [PKE 实验文档](../pke-doc/)

## 许可证

本项目遵循原项目的许可证。实验代码仅用于学习目的。

---

**Author**: Elysiasd  
**Course**: 操作系统 (2025)  
**University**: 华中科技大学
