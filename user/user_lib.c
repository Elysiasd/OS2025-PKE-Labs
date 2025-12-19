/*
 * The supporting library for applications.
 * Actually, supporting routines for applications are catalogued as the user 
 * library. we don't do that in PKE to make the relationship between application 
 * and user library more straightforward.
 */

#include "user_lib.h"
#include "util/types.h"
#include "util/snprintf.h"
#include "kernel/syscall.h"

int do_user_call(uint64 sysnum, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6,
                 uint64 a7) {
  int ret;

  // explicitly load arguments into registers to ensure correct passing
  register uint64 a0_reg asm("a0") = sysnum;
  register uint64 a1_reg asm("a1") = a1;
  register uint64 a2_reg asm("a2") = a2;
  register uint64 a3_reg asm("a3") = a3;
  register uint64 a4_reg asm("a4") = a4;
  register uint64 a5_reg asm("a5") = a5;
  register uint64 a6_reg asm("a6") = a6;
  register uint64 a7_reg asm("a7") = a7;

  asm volatile(
      "ecall\n"
      : "+r"(a0_reg)
      : "r"(a1_reg), "r"(a2_reg), "r"(a3_reg), "r"(a4_reg), "r"(a5_reg), "r"(a6_reg), "r"(a7_reg)
      : "memory");

  ret = a0_reg;
  return ret;
}

//
// printu() supports user/lab1_1_helloworld.c
//
int printu(const char* s, ...) {
  va_list vl;
  va_start(vl, s);

  char out[256];  // fixed buffer size.
  int res = vsnprintf(out, sizeof(out), s, vl);
  va_end(vl);
  const char* buf = out;
  size_t n = res < sizeof(out) ? res : sizeof(out);

  // make a syscall to implement the required functionality.
  return do_user_call(SYS_user_print, (uint64)buf, n, 0, 0, 0, 0, 0);
}

//
// applications need to call exit to quit execution.
//
int exit(int code) {
  return do_user_call(SYS_user_exit, code, 0, 0, 0, 0, 0, 0); 
}

//
// lib call to print_backtrace
//
void print_backtrace(int depth) {
  do_user_call(SYS_user_print_backtrace, depth, 0, 0, 0, 0, 0, 0);
}
