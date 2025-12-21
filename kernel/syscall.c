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
#include "sched.h"

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
  // reclaim the current process, and reschedule. added @lab3_1
  free_process( current );
  
  // If parent is blocked waiting, wake it up. added @lab3_challenge1
  if (current->parent && current->parent->status == BLOCKED) {
    insert_to_ready_queue(current->parent);
  }
  
  schedule();
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va;
  // if there are previously reclaimed pages, use them first (this does not change the
  // size of the heap)
  if (current->user_heap.free_pages_count > 0) {
    va =  current->user_heap.free_pages_address[--current->user_heap.free_pages_count];
    assert(va < current->user_heap.heap_top);
  } else {
    // otherwise, allocate a new page (this increases the size of the heap by one page)
    va = current->user_heap.heap_top;
    current->user_heap.heap_top += PGSIZE;

    current->mapped_info[HEAP_SEGMENT].npages++;
  }
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  // add the reclaimed page to the free page list
  current->user_heap.free_pages_address[current->user_heap.free_pages_count++] = va;
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current );
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield() {
  // TODO (lab3_2): implment the syscall of yield.
  // hint: the functionality of yield is to give up the processor. therefore,
  // we should set the status of currently running process to READY, insert it in
  // the rear of ready queue, and finally, schedule a READY process to run.
  insert_to_ready_queue(current);
  schedule();

  return 0;
}

//
// kerenl entry point of wait. added @lab3_challenge1
//
extern process procs[NPROC];
ssize_t sys_user_wait(int pid) {
  // Check if pid is valid
  if (pid < -1 || pid >= NPROC) {
    return -1;
  }
  
  if (pid > 0) {
    // Wait for specific child process
    if (procs[pid].status == FREE || procs[pid].parent != current) {
      // Invalid pid or not a child of current process
      return -1;
    }
    
    // Wait until the child process exits (becomes ZOMBIE)
    while (procs[pid].status != ZOMBIE) {
      current->status = BLOCKED;
      schedule();
    }
    
    // Child has exited, reclaim its resources and return its pid
    procs[pid].status = FREE;
    return pid;
  } else {
    // pid == -1: wait for any child process
    while (1) {
      int has_children = 0;
      for (int i = 0; i < NPROC; i++) {
        if (procs[i].parent == current && procs[i].status != FREE) {
          has_children = 1;
          if (procs[i].status == ZOMBIE) {
            // Found a zombie child, reclaim and return its pid
            procs[i].status = FREE;
            return i;
          }
        }
      }
      
      if (!has_children) {
        // No children to wait for
        return -1;
      }
      
      // Has children but none are ZOMBIE yet, block and wait
      current->status = BLOCKED;
      schedule();
    }
  }
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
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    case SYS_user_wait:
      return sys_user_wait(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
