# 操作系统实验报告

## 1. 实验概览
本实验报告涵盖了从基础的系统调用、异常处理，到内存管理、进程管理，以及文件系统的完整实现过程。通过逐步完善 RISC-V Proxy Kernel (PKE)，深入理解了操作系统的核心机制。

## 2. 实验内容与完成情况

### Lab 1: 中断、异常与系统调用
*   **Lab 1-1: 系统调用 (System Calls)**
    *   **提交信息**: `Lab1_1: Implemented system call handling by invoking do_syscall in handle_syscall`
    *   **内容**: 实现了基础的系统调用处理逻辑，通过 `handle_syscall` 调用 `do_syscall` 分发处理。
*   **Lab 1-2: 异常处理 (Exceptions)**
    *   **提交信息**: `Lab1_2: Implemented illegal instruction handling by calling handle_illegal_instruction in handle_mtrap`
    *   **内容**: 实现了非法指令异常的捕获与处理。
*   **Lab 1-3: 中断处理 (Interrupts)**
    *   **提交信息**: `Lab1_3: 在 handle_mtimer_trap 中增加 g_ticks 并清除 sip 寄存器中的 SIP_SSIP 位`
    *   **内容**: 实现了时钟中断处理，维护系统 tick 计数。
*   **Lab 1-Challenge 1: 栈回溯 (Backtrace)**
    *   **提交信息**: `完成Lab1_Challenge1: 实现用户程序调用栈回溯功能`
    *   **内容**: 实现了 `print_backtrace`，通过解析栈帧和 ELF 符号表，打印函数调用链，辅助调试。

### Lab 2: 内存管理
*   **Lab 2-1: 页表 (Page Tables)**
    *   **提交信息**: `完成 Lab2_1: 实现虚实地址转换`
    *   **内容**: 实现了虚拟地址到物理地址的转换逻辑。
*   **Lab 2-2: 内存分配 (Memory Allocation)**
    *   **提交信息**: `完成 Lab2_2: 实现简单内存分配和回收`
    *   **内容**: 实现了基础的物理内存分配与释放机制。
*   **Lab 2-3: 缺页异常 (Page Faults)**
    *   **提交信息**: `完成 Lab2_3: 实现缺页异常处理`
    *   **优化**: `优化 Lab2 实现：简化页面错误处理和内存管理逻辑`
    *   **内容**: 实现了缺页异常处理程序，支持按需分页。

### Lab 3: 进程管理
*   **Lab 3-1: 进程创建 (Fork)**
    *   **提交信息**: `完成 Lab3_1: 实现进程 fork，通过共享代码段而非复制实现子进程创建`
    *   **优化**: `Lab3_1: 优化实现，使用 map_pages 一次性映射整个代码段`
    *   **内容**: 实现了 `fork` 系统调用，通过复制父进程的元数据和栈，并共享代码段来创建子进程。
*   **Lab 3-2: 进程切换 (Yield)**
    *   **提交信息**: `完成 Lab3_2: 实现 yield 系统调用，支持进程主动释放CPU`
    *   **优化**: `优化 Lab3_2: 简化 yield 实现，移除冗余的状态设置`
    *   **内容**: 实现了 `yield` 系统调用，允许进程主动让出 CPU 进入就绪状态。
*   **Lab 3-3: 进程调度 (Scheduling)**
    *   **提交信息**: `完成 Lab3_3: 实现循环轮转调度，基于时间片的抢占式调度`
    *   **修复**: `修复 Lab3_3: 使用正确的 process.c 实现`
    *   **内容**: 实现了基于时间片轮转 (RR) 的进程调度算法。
*   **Lab 3-Challenge 1: 等待子进程 (Wait)**
    *   **提交信息**: `完成 Lab3_Challenge1: 实现 wait 系统调用和数据段复制`
    *   **内容**: 实现了 `wait` 系统调用，允许父进程等待子进程结束并回收资源。

### Lab 4: 文件系统
*   **Lab 4-1: 文件创建 (File Creation)**
    *   **提交信息**: `完成 Lab4_1: 实现 rfs_create 中 disk inode 的初始化`
    *   **内容**: 在 `kernel/rfs.c` 中实现了 `rfs_create` 函数，正确初始化了新文件的磁盘 inode (大小、类型、链接数等)。
*   **Lab 4-2: 目录读取 (Directory Reading)**
    *   **提交信息**: `完成 Lab4_2: 实现 rfs_readdir 中目录项的读取`
    *   **内容**: 在 `kernel/rfs.c` 中实现了 `rfs_readdir` 函数，能够从目录文件中读取目录项并返回给用户态。
*   **Lab 4-3: 硬链接 (Hard Links)**
    *   **提交信息**: `完成 Lab4_3: 实现 rfs_link 硬链接功能`
    *   **内容**: 在 `kernel/rfs.c` 中实现了 `rfs_link` 函数，支持为现有文件创建硬链接，增加了 inode 引用计数并添加了新的目录项。
*   **Lab 4-Challenge 1: 相对路径 (Relative Paths)**
    *   **提交信息**: `完成 Lab4_Challenge1: 实现相对路径支持，包括 pwd、cd 命令及 . 和 .. 路径解析`
    *   **内容**:
        *   新增系统调用 `SYS_user_rcwd` (pwd) 和 `SYS_user_ccwd` (cd)。
        *   在 `kernel/proc_file.c` 中维护进程的当前工作目录 (`cwd`)。
        *   修改 `kernel/vfs.c` 中的 `lookup_final_dentry`，支持解析 `.` (当前目录) 和 `..` (父目录)。
        *   更新 VFS 接口，支持基于 `cwd` 的相对路径查找。

## 3. 总结
通过一系列实验，成功构建了一个功能相对完善的 Proxy Kernel，涵盖了操作系统核心功能的实现。Git 日志清晰地反映了开发过程中的功能迭代、Bug 修复和代码优化过程。
