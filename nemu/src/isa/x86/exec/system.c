#include "cpu/exec.h"

make_EHelper(lidt)
{
  rtl_lm(&s0, &id_dest->addr, 2);
  IDTR(limit) = (unsigned short)s0 & 0xffff;
  rtl_addi(&id_dest->addr, &id_dest->addr, 2);
  rtl_lm(&s0, &id_dest->addr, 4);
  if (decinfo.isa.is_operand_size_16)
  {
    IDTR(base) = (0xffffff & s0);
  }
  else
  {
    IDTR(base) = s0;
  }
  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr)
{
  switch (id_dest->reg)
  {
  case 0:
    cpu.cr0 = id_src->val;
    break;
  case 3:
    cpu.cr3 = id_src->val;
    break;
  default:
    assert(0);
  }

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r)
{
  switch (id_src->reg)
  {
  case 0:
    t0 = cpu.cr0;
    break;
  case 3:
    t0 = cpu.cr3;
    break;
  default:
    assert(0);
  }
  operand_write(id_dest, &t0);

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif
}

void raise_intr(uint32_t NO, vaddr_t ret_addr);

make_EHelper(int)
{
  raise_intr(id_dest->val, *pc);
  print_asm("int %s", id_dest->str);
  difftest_skip_dut(1, 2);
}

make_EHelper(iret)
{
  // TODO();
  //中断返回
  rtl_pop(&s0);
  // printf("nemu jmp to %x\n", s0);
  rtl_j(s0);
  rtl_pop(&cpu.cs);
  rtl_pop(&cpu.eflag);
  cpu.eflags.IF = 1;
  print_asm("iret");
}

uint32_t pio_read_l(ioaddr_t);
uint32_t pio_read_w(ioaddr_t);
uint32_t pio_read_b(ioaddr_t);
void pio_write_l(ioaddr_t, uint32_t);
void pio_write_w(ioaddr_t, uint32_t);
void pio_write_b(ioaddr_t, uint32_t);

make_EHelper(in)
{
  switch (id_src->width)
  {
  case 1:
    t2 = pio_read_b(id_src->val);
    break;
  case 2:
    t2 = pio_read_w(id_src->val);
    break;
  case 4:
    t2 = pio_read_l(id_src->val);
    break;
  default:
    break;
  }
  operand_write(id_dest, &t2);
  print_asm_template2(in);

#ifdef DIFF_TEST
  difftest_skip_ref();
#endif
}

make_EHelper(out)
{
  switch (id_src->width)
  {
  case 1:
    pio_write_b(id_dest->val, id_src->val);
    break;
  case 2:
    pio_write_w(id_dest->val, id_src->val);
    break;
  case 4:
    pio_write_l(id_dest->val, id_src->val);
    break;
  default:
    break;
  }

  print_asm_template2(out);

#ifdef DIFF_TEST
  difftest_skip_ref();
#endif
}
