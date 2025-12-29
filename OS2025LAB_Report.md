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

## 3. 实验过程记录与截图
> 请在此处插入实验过程的截图，例如：
> 1. 本地代码开发界面的截图。
> 2. 调试过程的截图（如 GDB 调试或打印日志）。
> 3. Docker 运行测试通过的截图。

*(在此处粘贴截图)*

## 4. Git 日志与截图
> **要求**：必须包含带有作者名字、时间信息的 `git log` 命令输出截屏。作者邮箱必须与头歌注册邮箱一致。

### 4.1 Git Log 文本输出
```text
* a9981d9 (HEAD -> lab4_challenge1_relativepath) Add OS2025LAB experiment report
* 60cd3c1 完成 Lab4_Challenge1: 实现相对路径支持，包括 pwd、cd 命令及 . 和 .. 路径解析
* dfdaaeb Merge lab4_3_hardlink into lab4_challenge1_relativepath
* 2a560dd (lab4_3_hardlink) 完成 Lab4_3: 实现 rfs_link 硬链接功能
* 1ffa47e Merge lab4_2_directory into lab4_3_hardlink
```

### 4.2 Git Log 截图
> 请在此处插入 `git log` 的终端截图，确保包含 Author 和 Date 信息。

*(在此处粘贴截图)*

## 5. AI 辅助实验记录 (Lab 4 Challenge 1)
> 本实验部分功能在 AI 辅助下完成。以下是交互过程记录、验证与分析。

### 5.1 交互过程
**1. 任务启动与代码分析**
*   **Prompt**: "继续实现lab4-挑战1实验，并且列出修改完的仓库与原仓库有哪些文件变化"
*   **AI 行为**:
    *   分析了 `kernel/vfs.c`, `kernel/proc_file.c`, `kernel/syscall.h` 等文件。
    *   确定了需要新增 `SYS_user_rcwd` (pwd) 和 `SYS_user_ccwd` (cd) 系统调用。
    *   设计了在 `proc_file_management` 结构中维护 `cwd` (当前工作目录) 的方案。

**2. 代码实现**
*   **AI 修改**:
    *   **`kernel/syscall.h`**: 注册了新的系统调用号。
    *   **`kernel/proc_file.c`**: 实现了 `do_rcwd` (反向查找路径) 和 `do_ccwd` (切换目录)。
    *   **`kernel/vfs.c`**: 修改 `lookup_final_dentry` 以支持 `.` 和 `..` 解析；修改 `vfs_open` 等函数以支持相对路径查找（若路径不以 `/` 开头，则从 `cwd` 开始）。
    *   **`user/user_lib.c`**: 实现了用户态接口。

**3. 编译与调试**
*   **Prompt**: "用docker进行测试"
*   **AI 行为**: 运行 Docker 容器进行编译和测试。
*   **问题出现**: 编译报错 `kernel/proc_file.c:245:5: error: implicit declaration of function 'strncpy'`。
*   **AI 分析**: 内核态缺少标准库支持，`strncpy` 未定义或未包含头文件。
*   **AI 修复**: 将 `strncpy` 替换为 `memcpy`，并手动处理字符串结束符。
*   **验证**: 再次运行 Docker 测试，所有测试用例（`pwd`, `cd`, 相对路径读写）均通过。

### 5.2 验证与分析
*   **验证结果**:
    ```text
    ======== Test 1: change current directory  ========
    cwd:/
    change current directory to ./RAMDISK0
    cwd:/RAMDISK0
    ...
    All tests passed!
    ```
*   **观察与分析**:
    *   AI 正确识别了相对路径实现的核心在于 VFS 层的路径解析逻辑 (`lookup_final_dentry`)。
    *   在处理 `..` (父目录) 时，AI 利用了 `dentry->parent` 指针，这是文件系统树状结构的自然特性。
    *   在内核态编程中，AI 能够意识到标准库的缺失（如 `strncpy` 问题）并给出替代方案 (`memcpy`)，体现了对内核环境的理解。
    *   通过 Docker 环境的快速迭代，验证了代码的正确性。

## 6. 总结
通过一系列实验，成功构建了一个功能相对完善的 Proxy Kernel，涵盖了操作系统核心功能的实现。Git 日志清晰地反映了开发过程中的功能迭代、Bug 修复和代码优化过程。

