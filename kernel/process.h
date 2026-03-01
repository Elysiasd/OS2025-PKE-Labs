#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"

typedef struct trapframe_t {
  // space to store context (all common registers)
  /* offset:0   */ riscv_regs regs;

  // process's "user kernel" stack
  /* offset:248 */ uint64 kernel_sp;
  // pointer to smode_trap_handler
  /* offset:256 */ uint64 kernel_trap;
  // saved user process counter
  /* offset:264 */ uint64 epc;

  // kernel page table. added @lab2_1
  /* offset:272 */ uint64 kernel_satp;
}trapframe;

// Memory Control Block: placed immediately before each heap allocation.
// All pointer fields store VIRTUAL addresses (user address space).
// Kernel must call user_va_to_pa() before dereferencing them. added @lab2_challenge2
typedef struct mcb_t {
    int size;            // usable data bytes in this block
    int used;            // 1=allocated, 0=free
    struct mcb_t *next;  // VA of next MCB header (NULL if tail)
    struct mcb_t *prev;  // VA of prev MCB header (NULL if head)
} mcb;

// the extremely simple definition of process, used for begining labs of PKE
typedef struct process_t {
  // pointing to the stack used in trap handling.
  uint64 kstack;
  // user page table
  pagetable_t pagetable;
  // trapframe storing the context of a (User mode) process.
  trapframe* trapframe;
  // heap management fields. added @lab2_challenge2
  uint64 heap_start;   // VA of heap region base (= USER_FREE_ADDRESS_START)
  uint64 heap_end;     // VA one-past the last mapped heap byte
  mcb   *heap_head;    // VA of the first MCB in the chain (NULL if empty)
}process;
// switch to run user app
void switch_to(process*);

// current running process
extern process* current;

// address of the first free page in our simple heap. added @lab2_2
extern uint64 g_ufree_page;

#endif
