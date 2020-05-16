#include "syscall.h"
#include "common.h"

_Context *do_syscall(_Context *c)
{
  uintptr_t a[4];
  a[0] = c->GPR1;
  // a[1] = c->GPR2;
  // a[2] = c->GPR3;
  // a[3] = c->GPR4;
  switch (a[0])
  {
  case SYS_exit:
    Log("exit");
    _halt(1);
    break;
  case SYS_yield:
    _yield();
    Log("syscall yield");
    c->GPRx = 0;
    break;
  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
