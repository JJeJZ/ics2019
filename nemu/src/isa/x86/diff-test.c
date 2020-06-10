#include "monitor/diff-test.h"
#include "nemu.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
  for (int i = R_EAX; i <= R_EDI; i++)
  {
    if (reg_l(i) != ref_r->gpr[i]._32)
    {
      printf("different at: %s\n", reg_name(i, 4));
      return false;
    }
  }
  if (cpu.pc != ref_r->pc)
  {
    printf("different at pc\n");
    return false;
  }
  //对eflag的check需要变化一下
  bool flag = true;
  // if (cpu.eflags.CF != ref_r->eflags.CF)
  // {
  //   printf("different at CF\n");
  //   flag = false;
  // }
  // if (cpu.eflags.OF != ref_r->eflags.OF)
  // {
  //   printf("different at OF\n");
  //   flag = false;
  // }
  // if (cpu.eflags.SF != ref_r->eflags.SF)
  // {
  //   printf("different at SF\n");
  //   flag = false;
  // }
  // if (cpu.eflags.ZF != ref_r->eflags.ZF)
  // {
  //   printf("different at ZF\n");
  //   flag = false;
  // }

  return flag;
}

#define ISA_DIFF_END1 0x7c00
#define ISA_DIFF_ST2 0x1000000
#define ISA_DIFF_END2 PMEM_SIZE

void isa_difftest_attach(void)
{
  char *mainargs = guest_to_host(0);
  ref_difftest_memcpy_from_dut(PC_START - IMAGE_START, mainargs, 0x7c00);
  mainargs = (char *)ISA_DIFF_ST2;
  ref_difftest_memcpy_from_dut(PC_START - IMAGE_START + ISA_DIFF_ST2, mainargs, ISA_DIFF_END2 - ISA_DIFF_ST2);
  ref_difftest_setregs(&cpu);
}
