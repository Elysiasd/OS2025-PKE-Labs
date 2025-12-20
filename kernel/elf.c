/*
 * routines that scan and load a (host) Executable and Linkable Format (ELF) file
 * into the (emulated) memory.
 */

#include "elf.h"
#include "string.h"
#include "riscv.h"
#include "spike_interface/spike_utils.h"

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
  
  // Load symbol table for backtrace support
  load_elf_symbols(&info);
}

//
// Global symbol table for backtrace
//
static elf_symbol *g_symtab = NULL;
static char *g_strtab = NULL;
static int g_symtab_count = 0;
static uint64 g_strtab_size = 0;

//
// Load symbol table from ELF file for backtrace
//
void load_elf_symbols(elf_info *info) {
  elf_ctx ctx;
  ctx.info = info;
  
  // Read ELF header again
  if (elf_fpread(&ctx, &ctx.ehdr, sizeof(ctx.ehdr), 0) != sizeof(ctx.ehdr)) {
    return;
  }
  
  // Allocate space for section headers
  uint64 shdrs_size = ctx.ehdr.shnum * sizeof(elf_section_header);
  elf_section_header *shdrs = (elf_section_header *)0x80800000;  // Use a safe memory region
  
  // Read all section headers
  if (elf_fpread(&ctx, shdrs, shdrs_size, ctx.ehdr.shoff) != shdrs_size) {
    return;
  }
  
  // Find .shstrtab (section header string table)
  elf_section_header *shstrtab_hdr = &shdrs[ctx.ehdr.shstrndx];
  char *shstrtab = (char *)0x80820000;
  if (elf_fpread(&ctx, shstrtab, shstrtab_hdr->size, shstrtab_hdr->offset) != shstrtab_hdr->size) {
    return;
  }
  
  // Find .symtab and .strtab sections
  elf_section_header *symtab_hdr = NULL;
  elf_section_header *strtab_hdr = NULL;
  
  for (int i = 0; i < ctx.ehdr.shnum; i++) {
    char *name = shstrtab + shdrs[i].name;
    if (strcmp(name, ".symtab") == 0) {
      symtab_hdr = &shdrs[i];
    } else if (strcmp(name, ".strtab") == 0) {
      strtab_hdr = &shdrs[i];
    }
  }
  
  if (!symtab_hdr || !strtab_hdr) {
    return;  // No symbol table found
  }
  
  // Load symbol table
  g_symtab_count = symtab_hdr->size / sizeof(elf_symbol);
  g_symtab = (elf_symbol *)0x80840000;  // Use a safe memory region
  if (elf_fpread(&ctx, g_symtab, symtab_hdr->size, symtab_hdr->offset) != symtab_hdr->size) {
    g_symtab = NULL;
    return;
  }
  
  // Load string table
  g_strtab_size = strtab_hdr->size;
  g_strtab = (char *)0x80880000;  // Use a safe memory region
  if (elf_fpread(&ctx, g_strtab, strtab_hdr->size, strtab_hdr->offset) != strtab_hdr->size) {
    g_strtab = NULL;
    g_symtab = NULL;
    return;
  }
}

//
// Find function name by address using the loaded symbol table
//
const char* find_function_name(uint64 addr) {
  if (!g_symtab || !g_strtab) {
    return NULL;
  }
  
  // Search for the function containing this address
  for (int i = 0; i < g_symtab_count; i++) {
    elf_symbol *sym = &g_symtab[i];
    
    // Check if this is a function symbol (STT_FUNC = 2)
    uint8 sym_type = sym->info & 0xf;
    if (sym_type == 2) {  // STT_FUNC
      // Check if address is within this function's range
      if (addr >= sym->value && addr < sym->value + sym->size) {
        // Make sure name index is valid
        if (sym->name < g_strtab_size) {
          return g_strtab + sym->name;
        }
      }
    }
  }
  
  return NULL;
}
