#include <proc.h>
#include <elf.h>
#include <fs.h>
#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);
/*
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
*/
static uintptr_t loader(PCB *pcb, const char *filename) {
  
  int fd = fs_open(filename, 0, 0);
  if (fd < 0) {
    panic("should not reach here");
  }
  Elf_Ehdr elf;

  assert(fs_read(fd, &elf, sizeof(elf)) == sizeof(elf));
  // 检查魔数
  assert(*(uint32_t *)elf.e_ident == 0x464c457f);
  
  Elf_Phdr phdr;
  for (int i = 0; i < elf.e_phnum; i++) {
    uint32_t base = elf.e_phoff + i * elf.e_phentsize;

    fs_lseek(fd, base, 0);
    assert(fs_read(fd, &phdr, elf.e_phentsize) == elf.e_phentsize);
    
    // 需要装载的段
    if (phdr.p_type == PT_LOAD) {

      char * buf_malloc = (char *)malloc(phdr.p_filesz);

      fs_lseek(fd, phdr.p_offset, 0);
      assert(fs_read(fd, buf_malloc, phdr.p_filesz) == phdr.p_filesz);
      
      memcpy((void*)phdr.p_vaddr, buf_malloc, phdr.p_filesz);
      memset((void*)phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
      
      free(buf_malloc);
    }
  }

  assert(fs_close(fd) == 0);
  
  return elf.e_entry;
}
void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  // Load the ELF executable from filename
  uintptr_t entry = loader(pcb, filename);
  Log("Loaded: %s, Jump to entry = %p", filename, entry);

  // Prepare the stack for the new process
  Area stack;
  stack.start = pcb->stack;
  stack.end = pcb->stack + STACK_SIZE;

  // Create the process context (ucontext) with the stack and entry point
  pcb->cp = ucontext(NULL, stack, (void(*)())entry);

  // Prepare the arguments (argv) and environment variables (envp) on the stack
  char **user_stack = (char**)stack.end;

  // Calculate the amount of space needed for argv, envp, and the strings themselves
  int argc = 0, envc = 0;
  size_t strings_space = 0;

  // Calculate for argv
  for (; argv && argv[argc]; argc++) {
    strings_space += strlen(argv[argc]) + 1;
  }
  // Calculate for envp
  for (; envp && envp[envc]; envc++) {
    strings_space += strlen(envp[envc]) + 1;
  }

  // Calculate the total size for the array pointers plus the strings
  size_t pointers_space = (argc + envc + 2) * sizeof(char*) + sizeof(uintptr_t); // +2 for the NULL terminators, +1 for argc
  user_stack = (char**)((char*)user_stack - (pointers_space + strings_space));
  
  // Copy the argument pointers and the strings to the stack
  char* strings_top = (char*)(user_stack + argc + envc + 2); // +2 for the NULL terminators, starts after the pointers
  for (int i = 0; i < argc; i++) {
    user_stack[i] = strings_top;
    strcpy(strings_top, argv[i]);
    strings_top += strlen(argv[i]) + 1;
  }
  user_stack[argc] = NULL; // NULL-terminate the argv array

  // Copy the environment variable pointers and the strings to the stack
  char** envp_user_stack = user_stack + argc + 1; // +1 to skip over the NULL terminator
  for (int i = 0; i < envc; i++) {
    envp_user_stack[i] = strings_top;
    strcpy(strings_top, envp[i]);
    strings_top += strlen(envp[i]) + 1;
  }
  envp_user_stack[envc] = NULL; // NULL-terminate the envp array

  // Store the initial stack pointer in the process context
  pcb->cp->GPRx = (uintptr_t)user_stack;
  Log("Initial stack pointer: %p", pcb->cp->GPRx);
}


