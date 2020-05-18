#include "monitor/expr.h"
#include "monitor/monitor.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
// #include "x86/include/isa/reg.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
  if (args == NULL)
  {
    cpu_exec(1);
  }
  else
  {
    int i = atoi(args);
    cpu_exec(i);
  }
  return 0;
}

static int cmd_info(char *args)
{
  if (args[0] == 'w')
  {
    print_wp();
  }
  else if (args[0] == 'r')
  {
    for (int i = R_EAX; i <= R_EDI; i++)
    {
      printf("%s 0x%08x\n", reg_name(i, 4), cpu.gpr[i]._32);
    }
    printf("%s 0x%08x\n", "eip", cpu.pc);
    printf("%s 0x%08x\n", "eflags", cpu.eflag);
    printf("%s 0x%08x\n", "cs", cpu.cs);
    // for (int i = R_AX; i < R_DI; i++)
    // {
    //   printf("%s 0x%04x\n", reg_name(i, 2), reg_w(i));
    // }
    // for (int i = R_AL; i < R_BH; i++)
    // {
    //   printf("%s 0x%02x\n", reg_name(i, 1), reg_b(i));
    // }
  }
  return 0;
}

static int cmd_p(char *args)
{
  bool success;
  uint32_t result = expr(args, &success);
  if (success)
  {
    printf("the result is %x\n", result);
    return 0;
  }
  return -1;
}
static int cmd_x(char *args)
{
  int n;
  char buf[128];
  sscanf(args, "%d %s", &n, buf);
  bool success;
  uint32_t addr = expr(buf, &success);
  if (!success)
    return -1;
  // 这里要把物理地址转虚拟地址

  for (int i = 0; i < n; i++)
  {
    printf("0x%08x : ", addr + i);
    printf("0x%02x\n", vaddr_read(addr + i, 1));
  }
  printf("\n");
  return 0;
}
static int cmd_w(char *args)
{
  bool success;
  int value = expr(args, &success);
  if (!success)
  {
    printf("error expression!\n");
    return 0;
  }
  WP *wp = new_wp();
  strcpy(wp->args, args);
  wp->isuse = true;
  wp->value = value;
  return 0;
}
static int cmd_d(char *args)
{
  int n = atoi(args);
  del_wp(n);
  return 0;
}

void difftest_detach();
void difftest_attach();
static int cmd_attach(char *args)
{
  difftest_attach();
  return 0;
}

static int cmd_detach(char *args)
{
  difftest_detach();
  return 0;
}

void isa_reg_save(FILE *fp);

static int cmd_save(char *path)
{
  //保存寄存器
  FILE *fp = fopen(path, "w");
  if (!fp)
  {
    printf("plese enter a valid path\n");
    return 1;
  }
  isa_reg_save(fp);
  fclose(fp);
  return 0;
}

static int cmd_load(char *path)
{
  //加载寄存器
  FILE *fp = fopen(path, "r");
  if (!fp)
  {
    printf("plese enter a valid path\n");
    return 1;
  }
  isa_reg_load(fp);
  fclose(fp);
  return 0;
}

static struct
{
  char *name;
  char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display informations about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "excute n step or null input excute 1 step", cmd_si},
    {"info", "print the state of register or the watch point", cmd_info},
    {"p", "calculate the value of the expr", cmd_p},
    {"x", "scan the memory and print the value at the physical addr", cmd_x},
    {"w", "add a watch point at the expr", cmd_w},
    {"d", "delete the Nth watch point ", cmd_d},
    {"attach", "begin different test", cmd_attach},
    {"detach", "stop  different test", cmd_detach}
    /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode)
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}
