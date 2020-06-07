#include <am.h>
#include <klib.h>
#include <x86.h>
static _Context *(*user_handler)(_Event, _Context *) = NULL;

void __am_irq0();
void __am_vecsys();
void __am_vectrap();
void __am_vecnull();
void __am_switch(_Context *c);

void print_context(_Context *c)
{
  void *begin = c;
  while (begin < (void *)c + sizeof(_Context))
  {
    printf("0x%x\n", *(int *)begin);
    begin += 4;
  }
  printf("--------------\n");
}

void __am_get_cur_as(_Context *c);

_Context *__am_irq_handle(_Context *c)
{
  _Context *next = c;
  // printf("current as :%x\n", c->as->ptr);
  __am_get_cur_as(c);
  // printf("current as :%x\n", c->as->ptr);
  // print_context(c);
  if (user_handler)
  {
    _Event ev = {0};
    switch (c->irq)
    {
    case 32:
      ev.event = _EVENT_IRQ_TIMER;
      break;
    case 0x80:
      ev.event = _EVENT_SYSCALL;
      break;
    case 0x81:
      ev.event = _EVENT_YIELD;
      break;
    
    default:
      ev.event = _EVENT_ERROR;
      break;
    }

    next = user_handler(ev, c);
    if (next == NULL)
    {
      next = c;
    }
    // printf("switch to eip : %x\n", next->eip);
  }
  // printf("switch to new address: %x\n", next->as->ptr);
  __am_switch(next);
  return next;
}

int _cte_init(_Context *(*handler)(_Event, _Context *))
{
  static GateDesc idt[NR_IRQ];
  // initialize IDT
  for (unsigned int i = 0; i < NR_IRQ; i++)
  {
    idt[i] = GATE(STS_TG32, KSEL(SEG_KCODE), __am_vecnull, DPL_KERN);
  }
  // ----------------------- interrupts ----------------------------
  idt[32] = GATE(STS_IG32, KSEL(SEG_KCODE), __am_irq0, DPL_KERN);
  // ---------------------- system call ----------------------------
  idt[0x80] = GATE(STS_TG32, KSEL(SEG_KCODE), __am_vecsys, DPL_USER);
  idt[0x81] = GATE(STS_TG32, KSEL(SEG_KCODE), __am_vectrap, DPL_KERN);
  set_idt(idt, sizeof(idt));
  // register event handler
  user_handler = handler;
  return 0;
}

/*
+---------------+ <---- stack.end
|               |
|    context    |
|               |
+---------------+ <--+
|               |    |
|               |    |
|               |    |
|               |    |
+---------------+    |
|       cp      | ---+
+---------------+ <---- stack.start
|               |
*/
_Context *_kcontext(_Area stack, void (*entry)(void *), void *arg)
{
  _Context *c = (_Context *)(stack.end - sizeof(_Context));
  memset(c, 0, sizeof(_Context));
  c->cs = 8;
  c->eflags = 2;
  c->eip = (uintptr_t)entry;
  return c;
}

void _yield()
{
  asm volatile("int $0x81");
}

int _intr_read()
{
  return 0;
}

void _intr_write(int enable)
{
}
