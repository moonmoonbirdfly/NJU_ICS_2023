/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>
//全局变量，表示是否为批处理模式。如果是批处理模式，程序可能会在没有用户交互的情况下运行一系列指令。默认是非批处理模式。
static int is_batch_mode = false;

//初始化正则表达式功能的函数声明。
void init_regex();

//初始化wp_pool功能的函数声明，wp_pool可能是工作进程池的意思，详细功能需要看实际代码。
void init_wp_pool();

/* 使用 readline 函数应用库从标准输入获取输入行. */
static char* rl_gets() {
  //静态变量，用于存储在readline中读取的行。
  static char *line_read = NULL;

  //如果非空，则释放上一次读取的行的内存。
  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  //读取行，行起始标记是"(nemu) "。
  line_read = readline("(nemu) ");

  //如果行读取成功且非空，则将该行添加到history中。
  if (line_read && *line_read) {
    add_history(line_read);
  }

  //返回读取的行。
  return line_read;
}

//表示CPU执行命令 'c' 的函数，args表示参数。
static int cmd_c(char *args) {
  //CPU执行，参数设为-1可能表示执行全部或者无限制数量的指令。
  cpu_exec(-1);
  //返回0，表示达到预期结果。
  return 0;
}

//表示CPU执行命令 'si' 的函数，args表示参数。
static int cmd_si(char *args){
  //声明一个变量作为执行的步数。
  int step=0;
  
  //如果没有参数传入，则运行步数为1。
  if(args==NULL) step=1;

  //否则，使用sscanf将参数转为整数作为运行步数。
  else sscanf(args,"%d",&step);
  
  //CPU执行，参数为步数。
  cpu_exec(step);
  
  //返回0，表示达到预期结果。
  return 0;
}

//表示命令 'q' 的函数，args表示参数，可能用于退出程序。
static int cmd_q(char *args) {
  //设置nemu状态为QUIT，可能是表示程序被退出的状态。
  nemu_state.state = NEMU_QUIT;

  //返回-1，可能表示一个错误或异常状态。
  return -1;
}




static int cmd_info(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("Usage: info r (registers) or info w (watchpoints)\n");
  } else {
    if (strcmp(arg, "r") == 0) {
      isa_reg_display();
    }  }
  
  return 0;
}



static int cmd_x(char *args){
char *arg1 = strtok(NULL, " ");
  if (arg1 == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  char *arg2 = strtok(NULL, " ");
  if (arg1 == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  int n = strtol(arg1, NULL, 10);
  vaddr_t expr = strtol(arg2, NULL, 16);

  int i, j;
  for (i = 0; i < n;) {
    printf(ANSI_FMT("%#018x: ", ANSI_FG_CYAN), expr);
    
    for (j = 0; i < n && j < 4; i++, j++) {
      word_t w = vaddr_read(expr, 4);
      expr += 4;
      printf("%#018x ", w);
    }
    puts("");
  }
  

return 0;
}

static int cmd_p(char* args) {
  bool success;
  word_t res = expr(args, &success);
  if (!success) {
    puts("invalid expression");
  } else {
    printf("%u\n%08x\n", res,res);
    
  }
  return 0;
}


static int cmd_help(char *args);



static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "run program", cmd_si},
  {"info","Registor detailed infomation",cmd_info}, 
  /* TODO: Add more commands */
  {"x","scan memory from expr by n bytes",cmd_x},
  {"p", "Usage: p EXPR. Calculate the expression, e.g. p $eax + 1", cmd_p }

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {//先判断指令表有无这个指令，有则向下      //看看返回值是否小于0，如果小于0则返回到main函数
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
