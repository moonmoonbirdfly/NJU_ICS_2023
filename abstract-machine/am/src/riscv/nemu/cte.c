#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context *__am_irq_handle(Context *c)
{
  // __am_get_cur_as(c);
  // display_context(c);
  // printf("pte is at %p\n", c->pdir);
  // printf("cause to interrupt is %d\n", c->GPR1);
  // display_context(c);
  if (user_handler)
  {
    // printf("the exception cause id is %d\n", c->mcause);
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

    // printf("event id is %d\n",ev.event);
    // printf("here\n");
    c = user_handler(ev, c);
    // printf("===============");
    // printf("mepc is %p\n", c->mepc);
    // printf("c is at %p\n",c);
    assert(c != NULL);
  }
  // printf("ret from interrupt is %p\n", c->GPRx);
  // ((void (*)())c->mepc)();
  // c->gpr[0] = 0;
  // printf("pte is at %p after change\n", c->mepc);
  // printf("sp is %p\n", c->gpr[2]);
  // display_context(c);
  assert(c->mepc >= 0x40000000 && c->mepc <= 0x88000000);
  // __am_switch(c);
  // printf("entry is %p\n", c->mepc);
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

