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
ssize_t sys_user_print_backtrace(uint64 depth) {
  // TODO (lab1_challenge1): implement the printing of call stack of the user application.
  uint64 s0 = current->trapframe->regs.s0;
  uint64 ra = current->trapframe->regs.ra;
  uint64 i = 0;

  // open the elf file
  elf_ctx elfloader;
  elf_info info;
  
  arg_buf arg_bug_msg;
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc) panic("You need to specify the application program!\n");
  
  info.f = spike_file_open(arg_bug_msg.argv[0], O_RDONLY, 0);
  info.p = current;
  
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");

  // find the .symtab and .strtab sections
  uint64 shoff = elfloader.ehdr.shoff;
  uint16 shnum = elfloader.ehdr.shnum;
  uint16 shstrndx = elfloader.ehdr.shstrndx;
  
  elf_sect_header shdr;
  elf_sect_header symtab_shdr;
  elf_sect_header strtab_shdr;
  elf_sect_header shstrtab_shdr;
  
  // get section header string table
  elf_fpread(&elfloader, &shstrtab_shdr, sizeof(shstrtab_shdr), shoff + shstrndx * sizeof(shdr));
  
  // allocate buffer for section header string table
  char *shstrtab = (char *)pmm_alloc();
  elf_fpread(&elfloader, shstrtab, shstrtab_shdr.size, shstrtab_shdr.offset);

  int symtab_found = 0;
  int strtab_found = 0;

  for (int k = 0; k < shnum; k++) {
    elf_fpread(&elfloader, &shdr, sizeof(shdr), shoff + k * sizeof(shdr));
    char *name = shstrtab + shdr.name;
    if (strcmp(name, ".symtab") == 0) {
      symtab_shdr = shdr;
      symtab_found = 1;
    } else if (strcmp(name, ".strtab") == 0) {
      strtab_shdr = shdr;
      strtab_found = 1;
    }
  }
  
  pmm_free(shstrtab);

  if (!symtab_found || !strtab_found) {
    panic("symtab or strtab not found\n");
  }

  // allocate buffer for symtab and strtab
  // assuming they fit in one page for now, or we need to read them entry by entry
  // symtab can be large, so let's read entry by entry
  
  // strtab can also be large, but we only need to read the name when we find the symbol
  // so we can just read the name from file when needed
  
  sprint("back trace the user app in the following:\n");

  while (i < depth && ra != 0) {
    // find the symbol name of ra
    // iterate over symbols
    uint64 symtab_size = symtab_shdr.size;
    uint64 symtab_offset = symtab_shdr.offset;
    uint64 sym_entsize = symtab_shdr.entsize;
    uint64 num_syms = symtab_size / sym_entsize;
    
    elf_sym sym;
    int found = 0;
    for (int k = 0; k < num_syms; k++) {
      elf_fpread(&elfloader, &sym, sizeof(sym), symtab_offset + k * sym_entsize);
      // check if ra is within the function
      if (sym.value <= ra && ra < sym.value + sym.size) {
        // found the symbol
        // get the name
        char name[256];
        // read the name from strtab
        // we need to read character by character until null terminator?
        // or just read a chunk
        // let's read a chunk
        elf_fpread(&elfloader, name, 256, strtab_shdr.offset + sym.name);
        sprint("%s\n", name);
        if (strcmp(name, "main") == 0) {
             goto cleanup;
        }
        found = 1;
        break;
      }
    }
    
    if (!found) {
      sprint("???\n");
    }

    i++;
    // update ra and s0
    // ra is at s0 - 8
    // s0 is at s0 - 16
    
    // In lab1, we are in bare mode, so virtual address == physical address
    // We can access user stack directly
    
    uint64 *stack_ptr = (uint64 *)s0;
    // check if stack_ptr is valid?
    // for now assume it is valid
    
    ra = *(stack_ptr - 1);
    s0 = *(stack_ptr - 2);
  }

cleanup:
  spike_file_close(info.f);
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
