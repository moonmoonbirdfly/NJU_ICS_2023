#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
 //printf("mcause  0x%08x\n\n",c->mcause);
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 0:
        ev.event=EVENT_YIELD;break;
      default: ev.event = EVENT_ERROR; break;
    }
    //user_handler是cte_init中注册的回调函数
    c = user_handler(ev, c);
    assert(c != NULL);
  }
  
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}

