#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb() {
  current = &pcb_boot;
}

int sys_execve(const char *fname, char *const argv[], char *const envp[]) {
    context_uload(current, fname, argv, envp);
    switch_boot_pcb();
    yield();
    return -1;
}
void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}
Context *kcontext(Area kstack, void (*entry)(void *), void *arg);
void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area stack;
  stack.start = pcb->stack;
  stack.end = pcb->stack + STACK_SIZE;
  pcb->cp = kcontext(stack, entry, arg);
 
}
void init_proc() {
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here
	const char filename[] = "/bin/file-test";
  naive_uload(NULL, filename);
}

Context* schedule(Context *prev) {
    current->cp = prev;
 
// always select pcb[0] as the new process
  current = ((current == &pcb[0]) ? &pcb[1] : &pcb[0]);
  
// then return the new context
  return current->cp;
}
