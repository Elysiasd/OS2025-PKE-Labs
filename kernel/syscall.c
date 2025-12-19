/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"
#include "elf.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// implement the SYS_user_print_backtrace syscall
//
ssize_t sys_user_print_backtrace(int depth) {
  // Get user stack pointer from trapframe
  uint64 user_sp = current->trapframe->regs.sp;
  uint64 user_fp = current->trapframe->regs.s0;
  
  // Each frame is 16 bytes (ra + fp)
  const int FRAME_SIZE = 16;
  
  for (int i = 0; i < depth; i++) {
    // Check if fp is valid
    if (user_fp == 0 || user_fp < 0x81000000 || user_fp >= 0x82000000) {
      break;
    }
    
    // Read return address from frame
    uint64 ra = *(uint64*)(user_fp - 8);
    
    // Find function name from symbol table
    const char* func_name = find_function_name(ra);
    if (func_name) {
      sprint("%s\n", func_name);
    }
    
    // Move to previous frame
    uint64 prev_fp = *(uint64*)(user_fp - 16);
    if (prev_fp == user_fp || prev_fp == 0) {
      break;
    }
    user_fp = prev_fp;
  }
  
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_print_backtrace:
      return sys_user_print_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
