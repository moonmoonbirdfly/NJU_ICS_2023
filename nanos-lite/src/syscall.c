#include <common.h>
#include "syscall.h"
#include "fs.h"
#include <proc.h>
#include <sys/time.h>
#include <stdint.h>

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; //#define GPR1 gpr[17] // a7
 //printf("执行到do_syscall,此时根据c->GPR1的值来判断属于哪个系统调用 c->GPR1=a7=%d\n",a[0]);
  switch (a[0]) {
    case SYS_exit:c->GPRx=0;printf("SYS_exit， do_syscall此时 c->GPRx=%d\n",c->GPRx);halt(c->GPRx);//SYS_exit系统调用
    case SYS_yield:printf("SYS_yield， do_syscall此时c->GPRx=%d\n",c->GPRx);yield(); //SYS_yield系统调用
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
