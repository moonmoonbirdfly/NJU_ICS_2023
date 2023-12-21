#include <common.h>
#include "syscall.h"
#include "fs.h"
#include <proc.h>
#include <sys/time.h>
#include <stdint.h>
int sys_write(int fd,intptr_t *buf, size_t count){
   if(fd==1||fd==2){
		  for (int i = 0; i < count; i++) {
		  putch(*((char*)buf + i));
			}
		 return count;
   }
   return 0;
}
int sys_execve(const char *fname, char *const argv[], char *const envp[]);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; //#define GPR1 gpr[17] // a7
  intptr_t ret=0;
 //printf("执行到do_syscall,此时根据c->GPR1的值来判断属于哪个系统调用 c->GPR2=a7=%d\n",a[0]);
  switch (a[0]) {
    case SYS_exit:c->GPRx=0;printf("SYS_exit， do_syscall此时 c->GPRx=%d\n",c->GPRx);halt(0);//SYS_exit系统调用
    case SYS_yield:printf("SYS_yield， do_syscall此时c->GPRx=%d\n",c->GPRx);yield(); //SYS_yield系统调用
    case SYS_brk:c->GPRx=0;break;  
   /* case SYS_write:ret=sys_write(c->GPR2,(void *)c->GPR3,(size_t)c->GPR4);break;*/
   case SYS_write:
        ret = fs_write(c->GPR2, (void *)c->GPR3, (size_t)c->GPR4);
        //Log("fs_write(%d, %p, %d) = %d", c->GPR2, c->GPR3, c->GPR4, ret);
        break;
   case SYS_open:
        ret = fs_open((const char *)c->GPR2, c->GPR3, c->GPR4);
        //Log("fs_open(%s, %d, %d) = %d",(const char *)c->GPR2, c->GPR3, c->GPR4, ret);
        break;
    case SYS_read:
        ret = fs_read(c->GPR2, (void *)c->GPR3, (size_t)c->GPR4);
        //Log("fs_read(%d, %p, %d) = %d", c->GPR2, c->GPR3, c->GPR4, ret);
        break;
    case SYS_close:
        ret = fs_close(c->GPR2);
        //Log("fs_close(%d, %d, %d) = %d", c->GPR2, c->GPR3, c->GPR4, ret);
        break;
    case SYS_lseek:
        ret = fs_lseek(c->GPR2, (size_t)c->GPR3, c->GPR4);
        //Log("fs_lseek(%d, %d, %d) = %d", c->GPR2, c->GPR3, c->GPR4, ret);
        break;     
        
		case SYS_execve:
        //Log("sys_execve(%s, %p, %p)", (const char *)c->GPR2, c->GPR3, c->GPR4);
        sys_execve((const char *)c->GPR2, (char * const*)c->GPR3, (char * const*)c->GPR4);
        while(1);
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  c->GPRx=ret;
}
