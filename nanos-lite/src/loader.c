#include "proc.h"
#include <elf.h>
#include <fs.h>

#ifdef __ISA_AM_NATIVE__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
Elf_Phdr pHeaders[20];

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;
#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

#define PAGE_SIZE 0x1000

static uintptr_t loader(PCB *pcb, const char *filename)
{
  int fd = fs_open(filename, 0, 0);
  Log("fd : %d, filename : %s", fd, filename);
  //读取文件头
  Elf_Ehdr elfHeader;
  size_t len = fs_read(fd, &elfHeader, sizeof(Elf_Ehdr));
  assert(len == sizeof(Elf_Ehdr));
  //读取段头
  fs_lseek(fd, elfHeader.e_phoff, SEEK_SET);
  fs_read(fd, pHeaders, elfHeader.e_phentsize * elfHeader.e_phnum);
  uint32_t paddr, vaddr=0x40000000;
  int pages = 0;
  for (int i = 0; i < elfHeader.e_phnum; i++)
  {
    if (pHeaders[i].p_type != PT_LOAD)
      continue;
    //将该段读取到制定的内存位置
    fs_lseek(fd, pHeaders[i].p_offset, SEEK_SET);
    pages = (pHeaders[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
    paddr = (uint32_t)new_page(pages);
    fs_read(fd, (void *)paddr, pHeaders[i].p_filesz);
    vaddr = pHeaders[i].p_vaddr;
    for (int j = 0; j < pages; j++)
    {
      //需要一页一页的映射
      // printf("[loader]pa:%x mapped on va:%x\n", paddr, vaddr);
      _map(&pcb->as, (void *)vaddr, (void *)paddr, _PROT_READ | _PROT_WRITE | _PROT_EXEC);
      vaddr += PAGE_SIZE;
      paddr += PAGE_SIZE;
    }
    // fs_read(fd, (void *)pHeaders[i].p_vaddr, pHeaders[i].p_filesz);
    //如果出现没对齐的情况把相应的内存区域清0
    if (pHeaders[i].p_filesz < pHeaders[i].p_memsz)
      memset((void *)(paddr + pHeaders[i].p_filesz), 0, pHeaders[i].p_memsz - pHeaders[i].p_filesz);
  }
  pcb->max_brk = (uintptr_t)vaddr;
  return elfHeader.e_entry;
}

void naive_uload(PCB *pcb, const char *filename)
{
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %x", entry);
  ((void (*)())entry)();
}

void context_kload(PCB *pcb, void *entry)
{
  _Area stack;
  stack.start = pcb->stack;
  stack.end = stack.start + sizeof(pcb->stack);
  pcb->cp = _kcontext(stack, entry, NULL);
}

void context_uload(PCB *pcb, const char *filename)
{
  _protect(&pcb->as);
  uintptr_t entry = loader(pcb, filename);
  _Area stack;
  stack.start = pcb->stack;
  stack.end = stack.start + sizeof(pcb->stack);
  pcb->cp = _ucontext(&pcb->as, stack, stack, (void *)entry, NULL);
  // printf("user eip : %x\n", pcb->cp->eip);
}
