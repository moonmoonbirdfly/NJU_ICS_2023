#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#define MSTATUS_MIE  0x8
static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    printf("__am_irq_handle中c->mcause为%d\n",c->mcause);
    switch (c->mcause) {
      case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:case 10:case 11:case 12:case 13:case 14:case 15:case 16:case 17:case 18:case 19:ev.event=EVENT_SYSCALL;break;
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
  uintptr_t mstatus;
  asm volatile("csrr %0, mstatus" : "=r"(mstatus));
  return (mstatus & MSTATUS_MIE) != 0;
}

void iset(bool enable) {
  uintptr_t mstatus;
  if (enable) {
    // Set the MIE bit to enable interrupts
    asm volatile("csrrs %0, mstatus, %1" : "=r"(mstatus) : "r"(MSTATUS_MIE));
  } else {
    // Clear the MIE bit to disable interrupts
    asm volatile("csrrc %0, mstatus, %1" : "=r"(mstatus) : "r"(MSTATUS_MIE));
  }
}

