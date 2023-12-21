#include <common.h>
void do_syscall(Context *c);
Context* schedule(Context *prev);
static Context *do_event(Event e, Context *c)
{
  // Log("event is %d\n",e.event);
  switch (e.event)
  {
  case EVENT_YIELD:printf("event ID=%d\nc->GPRx=%d\n",e.event,c->GPRx);halt(0);printf("执行了halt之后");break;
  
  case EVENT_SYSCALL:
    do_syscall(c);
    break;
  case EVENT_IRQ_TIMER: 
      c = schedule(c);
      assert(c != NULL);        
      break;
  default:
    panic("Unhandled event ID = %d", e.event);
  }
  return c;
}
void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
