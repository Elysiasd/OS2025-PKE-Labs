# PKE Lab1 实验报告

## 实验环境
- 操作系统：Windows
- Docker 镜像：tjr9098/amd64_pke_mirrors:1.0
- RISC-V 工具链：riscv64-unknown-elf-gcc
- 模拟器：Spike RISC-V ISA Simulator

## Lab1_1: 系统调用

### 任务
实现系统调用机制，使用户程序能够通过 `ecall` 指令调用内核服务。

### 实现
在 [kernel/strap.c](kernel/strap.c#L145) 的 `handle_syscall()` 函数中：
```c
tf->regs.a0 = do_syscall(tf->regs.a0, tf->regs.a1, tf->regs.a2, 
                         tf->regs.a3, tf->regs.a4, tf->regs.a5, 
                         tf->regs.a6, tf->regs.a7);
```

### 原理
- 用户程序执行 `ecall` 指令触发系统调用异常
- CPU 切换到 S-mode（监管模式）
- 内核从 trapframe 中提取系统调用号（a0）和参数（a1-a7）
- 调用 `do_syscall()` 执行相应的系统调用处理函数
- 将返回值写入 trapframe 的 a0 寄存器

## Lab1_2: 异常处理

### 任务
处理非法指令异常，避免系统崩溃。

### 实现
在 [kernel/machine/mtrap.c](kernel/machine/mtrap.c) 的 `handle_mtrap()` 函数中：
```c
case CAUSE_ILLEGAL_INSTRUCTION:
  handle_illegal_instruction(regs);
  break;
```

### 原理
- CPU 遇到非法指令时触发异常，进入 M-mode（机器模式）
- 根据 `mcause` 寄存器判断异常类型
- 调用 `handle_illegal_instruction()` 打印错误信息并终止进程

## Lab1_3: 时钟中断

### 任务
处理定时器中断，实现时钟滴答计数。

### 实现
在 [kernel/strap.c](kernel/strap.c) 中实现 `handle_mtimer_trap()`：
```c
void handle_mtimer_trap() {
  sprint("Tik\n");
  g_ticks++;
  
  // Clear timer interrupt by writing to sip register
  write_csr(sip, 0);
}
```

### 原理
- Spike 模拟器定期产生定时器中断
- 中断发生时，CPU 跳转到中断处理程序
- 增加全局计数器 `g_ticks`
- 清除 `sip` 寄存器的中断待决位，允许下次中断

## Lab1_Challenge1: 调用栈回溯

### 任务
实现 `print_backtrace()` 系统调用，打印用户程序的函数调用链。

### 关键实现

#### 1. 系统调用接口
在 [user/user_lib.h](user/user_lib.h) 和 [user/user_lib.c](user/user_lib.c) 中添加：
```c
int print_backtrace(int depth);
```

在 [kernel/syscall.h](kernel/syscall.h) 中定义系统调用号：
```c
#define SYS_user_print_backtrace 66
```

#### 2. 栈帧遍历
在 [kernel/syscall.c](kernel/syscall.c) 中实现：
```c
ssize_t sys_user_print_backtrace(int depth) {
  // 获取用户态的帧指针
  uint64 user_fp = current->trapframe->regs.s0;
  
  // 跳过 do_user_call 的栈帧（32字节帧，prev_fp 在 fp-8）
  uint64 caller_fp = *(uint64*)(user_fp - 8);
  
  uint64 fp = caller_fp;
  for (int i = 0; i < depth && fp != 0; i++) {
    // 检查 fp 是否有效
    if (fp < 0x81000000 || fp > 0x81100000) break;
    
    // 读取返回地址（16字节帧，ra 在 fp-8）
    uint64 ra = *(uint64*)(fp - 8);
    
    // 查找并打印函数名
    const char* func_name = find_function_name(ra);
    if (func_name) {
      sprint("%s\n", func_name);
    }
    
    // 移动到前一个栈帧（prev_fp 在 fp-16）
    uint64 prev_fp = *(uint64*)(fp - 16);
    if (prev_fp == 0 || prev_fp <= fp || prev_fp > 0x81100000) break;
    
    fp = prev_fp;
  }
  
  return 0;
}
```

#### 3. ELF 符号表解析
在 [kernel/elf.h](kernel/elf.h) 中定义符号表结构：
```c
typedef struct elf_section_header {
  uint32 name;
  uint32 type;
  uint64 flags;
  uint64 addr;
  uint64 offset;
  uint64 size;
  uint32 link;
  uint32 info;
  uint64 addralign;
  uint64 entsize;
} elf_section_header;

typedef struct elf_symbol {
  uint32 name;    // String table index
  uint8 info;     // Type and binding
  uint8 other;    // Visibility
  uint16 shndx;   // Section index
  uint64 value;   // Symbol value (address)
  uint64 size;    // Symbol size
} elf_symbol;
```

在 [kernel/elf.c](kernel/elf.c) 中实现符号表加载：
```c
void load_elf_symbols(elf_info *ctx) {
  // 读取段头表
  // 查找 .symtab 和 .strtab 段
  // 将符号表和字符串表加载到内存
  // 保存到全局变量 g_symtab 和 g_strtab
}

const char* find_function_name(uint64 addr) {
  // 遍历符号表
  // 查找地址在 [symbol.value, symbol.value + symbol.size) 范围内的函数符号
  // 返回函数名称
}
```

### RISC-V 栈帧结构

#### 标准 16 字节栈帧（f1-f8, print_backtrace）
```
高地址
+---------------+
|   prev s0     | <- s0 - 16
+---------------+
|      ra       | <- s0 - 8
+---------------+  <- s0 (frame pointer)
|   局部变量    |
+---------------+  <- sp (stack pointer)
低地址
```

栈帧初始化代码：
```asm
addi sp, sp, -16      # 分配 16 字节
sd   ra, 8(sp)        # 保存返回地址
sd   s0, 0(sp)        # 保存前一个 s0
addi s0, sp, 16       # 设置当前 s0
```

#### do_user_call 的 32 字节栈帧
```
高地址
+---------------+
|   prev s0     | <- s0 - 8
+---------------+  <- s0 (frame pointer)
|   其他数据    |
+---------------+
|      ra       |
+---------------+  <- sp (stack pointer)
低地址
```

栈帧初始化代码：
```asm
addi sp, sp, -32      # 分配 32 字节
sd   s0, 24(sp)       # 保存前一个 s0 在 sp+24
addi s0, sp, 32       # 设置当前 s0 = sp + 32
```

### 调试过程

#### 问题1：prev_fp 读取为 0
**现象**：回溯只显示一个栈帧就停止

**原因**：从 `do_user_call` 的栈帧读取 prev_fp 时使用了错误的偏移量（-16），
而 `do_user_call` 是 32 字节栈帧，prev_fp 应该在 fp-8 位置。

**解决**：跳过 `do_user_call` 栈帧，从调用者（f8）开始遍历

#### 问题2：返回地址与函数名不匹配
**现象**：`find_function_name()` 返回 NULL

**原因**：
1. 返回地址是函数调用后的下一条指令地址，不是函数入口地址
2. 符号查找时需要检查地址是否在函数范围内

**解决**：在 `find_function_name()` 中使用范围匹配：
```c
if (addr >= sym->value && addr < sym->value + sym->size)
```

### 测试结果

#### 测试1：print_backtrace(7)
```
f8
f7
f6
f5
f4
f3
f2
```
✅ 正确打印 7 层调用栈

#### 测试2：print_backtrace(3)
```
f8
f7
f6
```
✅ 正确打印 3 层调用栈

#### 测试3：print_backtrace(100)
```
f8
f7
f6
f5
f4
f3
f2
f1
main
```
✅ 正确打印完整调用链（9 层），自动停止于 main

## 实验总结

### 收获
1. 深入理解了 RISC-V 的陷阱处理机制（trap、exception、interrupt）
2. 掌握了系统调用的实现原理和调用约定
3. 学习了 RISC-V 栈帧布局和函数调用规范
4. 熟悉了 ELF 文件格式和符号表解析
5. 提高了裸机调试能力

### 技术要点
- **系统调用**：通过 `ecall` 指令陷入内核，使用 a0-a7 传递参数和返回值
- **栈帧遍历**：利用帧指针（s0/fp）链式结构回溯调用栈
- **符号解析**：从 ELF 文件的 .symtab 段加载符号信息，映射地址到函数名
- **内存安全**：在 Lab1 的 bare mode 下，虚拟地址等于物理地址，需注意访问范围

### 实验心得
- 调试时使用 `objdump -d` 分析汇编代码非常有帮助
- 理解不同函数的栈帧大小差异是关键（do_user_call 32字节 vs 其他函数 16字节）
- Docker 容器化开发环境提高了实验效率和可重复性
