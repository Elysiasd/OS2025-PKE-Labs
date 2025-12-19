/*
 * routines that scan and load a (host) Executable and Linkable Format (ELF) file
 * into the (emulated) memory.
 */

#include "elf.h"
#include "string.h"
#include "riscv.h"
#include "spike_interface/spike_utils.h"

typedef struct elf_info_t {
  spike_file_t *f;
  process *p;
} elf_info;

//
// the implementation of allocater. allocates memory space for later segment loading
//
static void *elf_alloc_mb(elf_ctx *ctx, uint64 elf_pa, uint64 elf_va, uint64 size) {
  // directly returns the virtual address as we are in the Bare mode in lab1_x
  return (void *)elf_va;
}

//
// actual file reading, using the spike file interface.
//
static uint64 elf_fpread(elf_ctx *ctx, void *dest, uint64 nb, uint64 offset) {
  elf_info *msg = (elf_info *)ctx->info;
  // call spike file utility to load the content of elf file into memory.
  // spike_file_pread will read the elf file (msg->f) from offset to memory (indicated by
  // *dest) for nb bytes.
  return spike_file_pread(msg->f, dest, nb, offset);
}

//
// init elf_ctx, a data structure that loads the elf.
//
elf_status elf_init(elf_ctx *ctx, void *info) {
  ctx->info = info;

  // load the elf header
  if (elf_fpread(ctx, &ctx->ehdr, sizeof(ctx->ehdr), 0) != sizeof(ctx->ehdr)) return EL_EIO;

  // check the signature (magic value) of the elf
  if (ctx->ehdr.magic != ELF_MAGIC) return EL_NOTELF;

  return EL_OK;
}

//
// load the elf segments to memory regions as we are in Bare mode in lab1
//
elf_status elf_load(elf_ctx *ctx) {
  // elf_prog_header structure is defined in kernel/elf.h
  elf_prog_header ph_addr;
  int i, off;

  // traverse the elf program segment headers
  for (i = 0, off = ctx->ehdr.phoff; i < ctx->ehdr.phnum; i++, off += sizeof(ph_addr)) {
    // read segment headers
    if (elf_fpread(ctx, (void *)&ph_addr, sizeof(ph_addr), off) != sizeof(ph_addr)) return EL_EIO;

    if (ph_addr.type != ELF_PROG_LOAD) continue;
    if (ph_addr.memsz < ph_addr.filesz) return EL_ERR;
    if (ph_addr.vaddr + ph_addr.memsz < ph_addr.vaddr) return EL_ERR;

    // allocate memory block before elf loading
    void *dest = elf_alloc_mb(ctx, ph_addr.vaddr, ph_addr.vaddr, ph_addr.memsz);

    // actual loading
    if (elf_fpread(ctx, dest, ph_addr.memsz, ph_addr.off) != ph_addr.memsz)
      return EL_EIO;
  }

  return EL_OK;
}

typedef union {
  uint64 buf[MAX_CMDLINE_ARGS];
  char *argv[MAX_CMDLINE_ARGS];
} arg_buf;

//
// returns the number (should be 1) of string(s) after PKE kernel in command line.
// and store the string(s) in arg_bug_msg.
//
static size_t parse_args(arg_buf *arg_bug_msg) {
  // HTIFSYS_getmainvars frontend call reads command arguments to (input) *arg_bug_msg
  long r = frontend_syscall(HTIFSYS_getmainvars, (uint64)arg_bug_msg,
      sizeof(*arg_bug_msg), 0, 0, 0, 0, 0);
  kassert(r == 0);

  size_t pk_argc = arg_bug_msg->buf[0];
  uint64 *pk_argv = &arg_bug_msg->buf[1];

  int arg = 1;  // skip the PKE OS kernel string, leave behind only the application name
  for (size_t i = 0; arg + i < pk_argc; i++)
    arg_bug_msg->argv[i] = (char *)(uintptr_t)pk_argv[arg + i];

  //returns the number of strings after PKE kernel in command line
  return pk_argc - arg;
}

//
// load the elf of user application, by using the spike file interface.
//
void load_bincode_from_host_elf(process *p) {
  arg_buf arg_bug_msg;

  // retrieve command line arguements
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc) panic("You need to specify the application program!\n");

  sprint("Application: %s\n", arg_bug_msg.argv[0]);

  //elf loading. elf_ctx is defined in kernel/elf.h, used to track the loading process.
  elf_ctx elfloader;
  // elf_info is defined above, used to tie the elf file and its corresponding process.
  elf_info info;

  info.f = spike_file_open(arg_bug_msg.argv[0], O_RDONLY, 0);
  info.p = p;
  // IS_ERR_VALUE is a macro defined in spike_interface/spike_htif.h
  if (IS_ERR_VALUE(info.f)) panic("Fail on openning the input application program.\n");

  // init elfloader context. elf_init() is defined above.
  if (elf_init(&elfloader, &info) != EL_OK)
    panic("fail to init elfloader.\n");

  // load elf. elf_load() is defined above.
  if (elf_load(&elfloader) != EL_OK) panic("Fail on loading elf.\n");

  // entry (virtual, also physical in lab1_x) address
  p->trapframe->epc = elfloader.ehdr.entry;

  // close the host spike file
  spike_file_close( info.f );

  sprint("Application program entry point (virtual address): 0x%lx\n", p->trapframe->epc);
}

//
// Global variables to store symbol table information
//
static elf_symbol *g_symtab = NULL;
static char *g_strtab = NULL;
static int g_symtab_count = 0;

//
// Load symbol table from ELF file
//
void load_symbol_table() {
  arg_buf arg_bug_msg;
  size_t argc = parse_args(&arg_bug_msg);
  if (!argc) return;

  spike_file_t *f = spike_file_open(arg_bug_msg.argv[0], O_RDONLY, 0);
  if (IS_ERR_VALUE(f)) return;

  elf_header ehdr;
  if (spike_file_pread(f, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
    spike_file_close(f);
    return;
  }

  // Read section headers
  elf_section_header shdr;
  elf_section_header strtab_shdr;
  elf_section_header symtab_shdr;
  int found_symtab = 0, found_strtab = 0;

  // First, read the section string table
  if (spike_file_pread(f, &shdr, sizeof(shdr), 
                       ehdr.shoff + ehdr.shstrndx * sizeof(shdr)) != sizeof(shdr)) {
    spike_file_close(f);
    return;
  }

  char *shstrtab = (char *)elf_alloc_mb(NULL, 0, 0, shdr.size);
  if (spike_file_pread(f, shstrtab, shdr.size, shdr.offset) != shdr.size) {
    spike_file_close(f);
    return;
  }

  // Find .symtab and .strtab sections
  for (int i = 0; i < ehdr.shnum; i++) {
    if (spike_file_pread(f, &shdr, sizeof(shdr), 
                         ehdr.shoff + i * sizeof(shdr)) != sizeof(shdr)) {
      continue;
    }

    const char *section_name = shstrtab + shdr.name;
    
    if (strcmp(section_name, ".symtab") == 0) {
      symtab_shdr = shdr;
      found_symtab = 1;
    } else if (strcmp(section_name, ".strtab") == 0) {
      strtab_shdr = shdr;
      found_strtab = 1;
    }
  }

  if (!found_symtab || !found_strtab) {
    spike_file_close(f);
    return;
  }

  // Load symbol table
  g_symtab_count = symtab_shdr.size / sizeof(elf_symbol);
  g_symtab = (elf_symbol *)elf_alloc_mb(NULL, 0, 0x80800000, symtab_shdr.size);
  if (spike_file_pread(f, g_symtab, symtab_shdr.size, symtab_shdr.offset) != symtab_shdr.size) {
    g_symtab = NULL;
    spike_file_close(f);
    return;
  }

  // Load string table
  g_strtab = (char *)elf_alloc_mb(NULL, 0, 0x80900000, strtab_shdr.size);
  if (spike_file_pread(f, g_strtab, strtab_shdr.size, strtab_shdr.offset) != strtab_shdr.size) {
    g_strtab = NULL;
    g_symtab = NULL;
    spike_file_close(f);
    return;
  }

  spike_file_close(f);
}

//
// Find function name by address
//
const char* find_function_name(uint64 addr) {
  // Load symbol table if not already loaded
  if (g_symtab == NULL || g_strtab == NULL) {
    load_symbol_table();
  }

  if (g_symtab == NULL || g_strtab == NULL) {
    return NULL;
  }

  // Search for the function containing the address
  for (int i = 0; i < g_symtab_count; i++) {
    elf_symbol *sym = &g_symtab[i];
    
    // Check if this is a function symbol (STT_FUNC = 2)
    if ((sym->info & 0xf) == 2) {
      if (addr >= sym->value && addr < sym->value + sym->size) {
        return g_strtab + sym->name;
      }
    }
  }

  return NULL;
}
