#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <nemu.h>
static Context* (*user_handler)(Event, Context*) = NULL;
void __am_get_cur_as(Context *c);
void __am_switch(Context *c);
Context* __am_irq_handle(Context *c) {
 //printf("mcause  %d \n",(int)c->mcause);
 //panic("mcause %d",(int)c->mcause);
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause)
    {
    case -1:
      ev.event = EVENT_YIELD;
      break;
    case 0 ... 19:
      ev.event = EVENT_SYSCALL;
      break;
    case 0x80000007:
      ev.event = EVENT_IRQ_TIMER;
      break;
    default:
      ev.event = EVENT_ERROR;
      break;
    }
    //user_handler是cte_init中注册的回调函数
    c = user_handler(ev, c);
    //assert(c != NULL);
  }
  assert(c->mepc >= 0x40000000 && c->mepc <= 0x88000000);
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

