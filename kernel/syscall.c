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
#include "pmm.h"
#include "vmm.h"
#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
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
// Heap management for lab2_challenge2_singlepageheap.
//
// Design contract:
//   - current->heap_head / mcb.next / mcb.prev all store VIRTUAL addresses.
//   - Kernel must call user_va_to_pa() before reading or writing any MCB field.
//   - Values returned to user are always virtual addresses.
//

// Convert a heap virtual address (non-NULL) to a kernel-accessible physical pointer.
static inline mcb *va_to_mcb(uint64 va) {
  return (mcb *)user_va_to_pa((pagetable_t)current->pagetable, (void *)va);
}

// Expand the heap by one physical page and add a single free MCB covering the
// whole new page.  VA of the new free block's header is returned; 0 on failure.
static uint64 heap_expand() {
  void *pa = alloc_page();
  if (!pa) return 0;

  uint64 va = current->heap_end;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE,
              (uint64)pa, prot_to_type(PROT_WRITE | PROT_READ, 1));
  current->heap_end += PGSIZE;

  // Build a free MCB that covers the entire new page minus its own header.
  mcb *blk = (mcb *)pa;   // physical pointer, same as va_to_mcb(va) for new page
  blk->size = PGSIZE - sizeof(mcb);
  blk->used = 0;
  blk->next = NULL;
  blk->prev = NULL;

  // Append the new free block to the tail of the MCB chain.
  if (current->heap_head == NULL) {
    current->heap_head = (mcb *)va;  // store VA
  } else {
    // Walk to the tail (all pointers are VAs).
    uint64 tail_va = (uint64)current->heap_head;
    mcb   *tail    = va_to_mcb(tail_va);
    while (tail->next != NULL) {
      tail_va = (uint64)tail->next;
      tail    = va_to_mcb(tail_va);
    }
    tail->next = (mcb *)va;          // store VA
    blk->prev  = (mcb *)tail_va;    // store VA
  }

  return va;
}

// better_malloc: allocate n bytes from the heap.  added @lab2_challenge2
uint64 sys_user_allocate_page(int n) {
  if (n <= 0) return 0;

  // Round n up to an 8-byte boundary so that every MCB header following the
  // data area remains naturally aligned for its 8-byte pointer members.
  n = (n + 7) & ~7;

  // ---- Pass 1: first-fit scan over existing free blocks ----
  uint64 blk_va = (uint64)current->heap_head;
  while (blk_va != 0) {
    mcb *blk = va_to_mcb(blk_va);

    if (!blk->used && blk->size >= n) {
      // If excess space is large enough to hold another MCB + ≥1 byte, split.
      if (blk->size >= n + (int)sizeof(mcb) + 1) {
        uint64 new_va  = blk_va + sizeof(mcb) + n;
        mcb   *new_blk = va_to_mcb(new_va);

        new_blk->size = blk->size - n - sizeof(mcb);
        new_blk->used = 0;
        new_blk->next = blk->next;
        new_blk->prev = (mcb *)blk_va;

        if (blk->next != NULL)
          va_to_mcb((uint64)blk->next)->prev = (mcb *)new_va;

        blk->next = (mcb *)new_va;
        blk->size = n;
      }
      blk->used = 1;
      return blk_va + sizeof(mcb);  // return VA of data area
    }
    blk_va = (uint64)blk->next;
  }

  // ---- Pass 2: no first-fit found ----
  // Walk to the tail of the MCB chain.
  uint64 tail_va = 0;
  uint64 cur_va  = (uint64)current->heap_head;
  while (cur_va != 0) {
    tail_va = cur_va;
    cur_va  = (uint64)va_to_mcb(cur_va)->next;
  }

  if (tail_va != 0 && !va_to_mcb(tail_va)->used) {
    // The last block is FREE but too small.
    // Strategy: extend the heap by mapping additional physical pages until the
    // tail block is large enough, then allocate directly from it.
    // This keeps the new allocation address-adjacent to previous ones (compact).
    uint64 data_va    = tail_va + sizeof(mcb);
    uint64 needed_end = data_va + n;

    while (current->heap_end < needed_end) {
      void *pa = alloc_page();
      if (!pa) panic("heap: out of physical memory\n");
      user_vm_map((pagetable_t)current->pagetable, current->heap_end, PGSIZE,
                  (uint64)pa, prot_to_type(PROT_WRITE | PROT_READ, 1));
      current->heap_end += PGSIZE;
    }

    // Update size to cover all newly mapped space starting from this block.
    mcb *tail  = va_to_mcb(tail_va);
    tail->size = current->heap_end - data_va;

    // Allocate from the now-extended tail block (split if possible).
    if (tail->size >= n + (int)sizeof(mcb) + 1) {
      uint64 new_va  = tail_va + sizeof(mcb) + n;
      mcb   *new_blk = va_to_mcb(new_va);
      new_blk->size  = tail->size - n - sizeof(mcb);
      new_blk->used  = 0;
      new_blk->next  = NULL;
      new_blk->prev  = (mcb *)tail_va;
      tail->next     = (mcb *)new_va;
      tail->size     = n;
    }
    tail->used = 1;
    return tail_va + sizeof(mcb);
  }

  // Last block is used (or chain is empty): append a brand-new free page.
  if (heap_expand() == 0)
    panic("heap_expand: out of memory\n");

  // The new free block was appended by heap_expand(); recurse to allocate.
  return sys_user_allocate_page(n);
}

// better_free: release the allocation whose data area starts at va. added @lab2_challenge2
uint64 sys_user_free_page(uint64 va) {
  if (va == 0) return 0;

  uint64 blk_va = va - sizeof(mcb);
  mcb   *blk    = va_to_mcb(blk_va);

  blk->used = 0;

  // Merge with successor if it is free.
  if (blk->next != NULL) {
    mcb *nxt = va_to_mcb((uint64)blk->next);
    if (!nxt->used) {
      blk->size += sizeof(mcb) + nxt->size;
      blk->next  = nxt->next;
      if (nxt->next != NULL)
        va_to_mcb((uint64)nxt->next)->prev = (mcb *)blk_va;
    }
  }

  // Merge with predecessor if it is free.
  if (blk->prev != NULL) {
    mcb *prv    = va_to_mcb((uint64)blk->prev);
    uint64 prv_va = (uint64)blk->prev;
    if (!prv->used) {
      prv->size += sizeof(mcb) + blk->size;
      prv->next  = blk->next;
      if (blk->next != NULL)
        va_to_mcb((uint64)blk->next)->prev = (mcb *)prv_va;
    }
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
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page(a1);
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
