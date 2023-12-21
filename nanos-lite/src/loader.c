#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
  // check valid elf
  assert((*(uint32_t *)ehdr.e_ident == 0x464c457f));

  Elf_Phdr phdr[ehdr.e_phnum];
  ramdisk_read(phdr, ehdr.e_phoff, sizeof(Elf_Phdr)*ehdr.e_phnum);
  for (int i = 0; i < ehdr.e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
      ramdisk_read((void*)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_memsz);
      // set .bss with zeros
      memset((void*)(phdr[i].p_vaddr+phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
    }
  }
  return ehdr.e_entry;
}


void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  Log("filename: %s", filename);
  uintptr_t entry = loader(pcb, filename);

  Area stack;
  stack.start = pcb->stack;
  stack.end = pcb->stack + STACK_SIZE;

  Log("stack.start: %p, stack.end: %p", stack.start, stack.end);
  Log("entry: %p", entry);

  // prepare entry point
  pcb->cp = ucontext(NULL, stack, (void(*)()) entry);

  // prepare argv and envp
  void *ustack_end = new_page(8);
  int space_count = 0; // unit: the size of pointer
  // TODO: unspecified part

  int argc = 0;
  if (argv) while (argv[argc]) {
      // Log("argv[%d]: %s", argc, argv[argc]); 
      argc++;
  }
  space_count += sizeof(uintptr_t); // for argc
  space_count += sizeof(uintptr_t) * (argc + 1); // for argv
  if (argv) for (int i = 0; i < argc; ++i) space_count += (strlen(argv[i]) + 1);

  int envpc = 0;
  if (envp) while (envp[envpc]) envpc++;
  space_count += sizeof(uintptr_t) * (envpc + 1); // for envp
  if (envp) for (int i = 0; i < envpc; ++i) space_count += (strlen(envp[i]) + 1);

  Log("argc: %d, envpc: %d, space_count: %d", argc, envpc, space_count);

  space_count += sizeof(uintptr_t); // for ROUNDUP
  Log("base before ROUNDUP: %p", ustack_end - space_count);
  uintptr_t *base = (uintptr_t *)ROUNDUP(ustack_end - space_count, sizeof(uintptr_t));
  uintptr_t *base_mem = base;
  Log("base after ROUNDUP: %p", base);

  *base = argc;
  base += 1;

  char *argv_temp[argc];
  char *envp_temp[envpc];
  base += (argc + 1) + (envpc + 1); // jump to string area
  char *string_area_curr = (char *)base;
  uintptr_t *string_area_curr_mem = (uintptr_t *)string_area_curr;

  for (int i = 0; i < argc; ++i) {
      strcpy(string_area_curr, argv[i]);
      argv_temp[i] = string_area_curr;
      string_area_curr += (strlen(argv[i]) + 1);
      Log("argv[%d]: %s, addr: %p", i, argv[i], argv_temp[i]);
  }

  for (int i = 0; i < envpc; ++i) {
      strcpy(string_area_curr, envp[i]);
      envp_temp[i] = string_area_curr;
      string_area_curr += (strlen(envp[i]) + 1);
      Log("envp[%d]: %s, addr: %p", i, envp[i], envp_temp[i]);
  }

  base -= (argc + 1) + (envpc + 1); // jump back

  for (int i = 0; i < argc; ++i) {
      *base = (uintptr_t)argv_temp[i];
      base += 1;
  }

  *base = (uintptr_t)NULL;
  base += 1;

  for (int i = 0; i < envpc; ++i) {
      *base = (uintptr_t)envp_temp[i];
      base += 1;
  }

  *base = (uintptr_t)NULL;
  base += 1;

  assert(string_area_curr_mem == base);

  pcb->cp->GPRx = (uintptr_t)base_mem;
}

