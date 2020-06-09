#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb()
{
  current = &pcb_boot;
}

void hello_fun(void *arg)
{
  int j = 1;
  while (1)
  {
    Log("Hello World from Nanos-lite for the %dth time!", j);
    j++;
    _yield();
  }
}

void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename);
void context_kload(PCB *pcb, void *entry);
void init_proc()
{
  memset(pcb, 0, sizeof(PCB) * MAX_NR_PROC);
  context_uload(&pcb[0], "/bin/pal");
  context_uload(&pcb[1], "/bin/hello");
  switch_boot_pcb();
}

int counter = 0;

_Context *schedule(_Context *prev)
{
  current->cp = prev;
  if (counter++ > 100)
  {
    current = &pcb[1];
    counter = 0;
  }
  else
  {
    current = &pcb[0];
  }
  return current->cp;
}
