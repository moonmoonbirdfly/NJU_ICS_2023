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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 2147483647

CPU_state cpu = {}; // 定义一个结构体变量，用于存放 CPU 的状态，例如寄存器的值
uint64_t g_nr_guest_inst = 0; // 定义一个全局变量，用于统计执行的指令数
static uint64_t g_timer = 0; // unit: us // 定义一个静态变量，用于记录模拟器的运行时间，单位是微秒
static bool g_print_step = false; // 定义一个静态变量，用于控制是否打印每条指令的信息

void device_update(); // 声明一个函数，用于更新设备的状态，例如键盘、鼠标、屏幕等

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) { // 定义一个静态函数，用于跟踪和对比测试指令，参数是一个解码结构体和动态的下一条指令的地址
#ifdef CONFIG_ITRACE_COND // 如果定义了 CONFIG_ITRACE_COND 这个宏，表示开启了指令跟踪的条件
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); } // 如果满足指令跟踪的条件，就把解码结构体中的日志缓冲区写入到日志文件中
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); } // 如果开启了打印每条指令的信息，就把解码结构体中的日志缓冲区输出到标准输出中
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc)); // 如果开启了对比测试，就调用 difftest_step 函数，传递当前指令的地址和动态的下一条指令的地址，用于和参考模拟器进行比较
}

static void exec_once(Decode *s, vaddr_t pc) { // 定义一个静态函数，用于执行一条指令，参数是一个解码结构体和指令的地址
  s->pc = pc; // 把指令的地址赋值给解码结构体中的 pc 变量
  s->snpc = pc; // 把指令的地址赋值给解码结构体中的 snpc 变量，表示静态的下一条指令的地址
  isa_exec_once(s); // 调用 isa_exec_once 函数，根据不同的 ISA 执行一条指令，更新解码结构体中的信息
  cpu.pc = s->dnpc; // 把解码结构体中的 dnpc 变量，表示动态的下一条指令的地址，赋值给 CPU 状态中的 pc 变量
#ifdef CONFIG_ITRACE // 如果定义了 CONFIG_ITRACE 这个宏，表示开启了指令跟踪
  char *p = s->logbuf; // 定义一个字符指针，指向解码结构体中的日志缓冲区
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc); // 把指令的地址格式化输出到日志缓冲区中，更新指针的位置
  int ilen = s->snpc - s->pc; // 计算指令的长度，等于静态的下一条指令的地址减去当前指令的地址
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val; // 定义一个字节指针，指向解码结构体中的指令的原始值
  for (i = ilen - 1; i >= 0; i --) { // 从高位到低位遍历指令的字节
    p += snprintf(p, 4, " %02x", inst[i]); // 把指令的每个字节以十六进制格式输出到日志缓冲区中，更新指针的位置
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4); // 根据不同的 ISA 定义指令的最大长度，x86 是 8 字节，其他是 4 字节
  int space_len = ilen_max - ilen; // 计算指令的空白长度，等于最大长度减去实际长度
  if (space_len < 0) space_len = 0; // 如果空白长度小于 0，就置为 0
  space_len = space_len * 3 + 1; // 计算空白占用的字符数，等于空白长度乘以 3 加 1，因为每个字节占两个字符和一个空格，最后还有一个空格
  memset(p, ' ', space_len); // 用空格填充日志缓冲区中的空白部分
  p += space_len; // 更新指针的位置

#ifndef CONFIG_ISA_loongarch32r // 如果没有定义 CONFIG_ISA_loongarch32r 这个宏，表示不支持 loongarch32r 这种 ISA
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte); // 声明一个函数，用于反汇编指令，参数是一个字符串指针，一个字符串的大小，一个指令的地址，一个指令的字节指针，一个指令的字节数
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p, // 调用反汇编函数，把反汇编的结果输出到日志缓冲区中，更新指针的位置
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r // 如果支持 loongarch32r 这种 ISA，就把日志缓冲区的第一个字符置为 '\0'，表示空字符串，因为上游的 llvm 不支持这种 ISA 的反汇编
#endif
#endif
}

static void execute(uint64_t n) { // 定义一个静态函数，用于执行 n 条指令，参数是一个无符号的 64 位整数
  Decode s; // 定义一个解码结构体变量，用于存放指令的解码信息
  for (;n > 0; n --) { // 用一个循环，从 n 到 0，每次减 1
    exec_once(&s, cpu.pc); // 调用 exec_once 函数，执行一条指令，传递解码结构体的指针和 CPU 状态中的 pc 变量
    g_nr_guest_inst ++; // 把全局变量 g_nr_guest_inst 加 1，表示执行的指令数增加
    trace_and_difftest(&s, cpu.pc); // 调用 trace_and_difftest 函数，跟踪和对比测试指令，传递解码结构体的指针和 CPU 状态中的 pc 变量
    if (nemu_state.state != NEMU_RUNNING) break; // 如果模拟器的状态不是运行中，就跳出循环
    IFDEF(CONFIG_DEVICE, device_update()); // 如果开启了设备模拟，就调用 device_update 函数，更新设备的状态
  }
}


static void statistic() { // 定义一个静态函数，用于打印模拟器的运行统计信息
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, "")); // 如果没有定义 CONFIG_TARGET_AM 这个宏，表示不是在 AM 平台上运行，就设置本地化的数字格式，例如千位分隔符
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64 // 定义一个宏，用于格式化无符号的 64 位整数，根据不同的平台，选择是否使用千位分隔符
  Log("host time spent = " NUMBERIC_FMT " us", g_timer); // 把全局变量 g_timer，表示模拟器的运行时间，以微秒为单位，格式化输出到日志中
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst); // 把全局变量 g_nr_guest_inst，表示执行的指令数，格式化输出到日志中
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer); // 如果模拟器的运行时间大于 0，就计算并输出模拟器的执行频率，等于指令数乘以 1000000 除以运行时间，以每秒为单位
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency"); // 如果模拟器的运行时间小于等于 0，就输出无法计算执行频率的信息
}

void assert_fail_msg() { // 定义一个函数，用于处理断言失败的情况
  isa_reg_display(); // 调用 isa_reg_display 函数，显示 CPU 的寄存器的值
  statistic(); // 调用 statistic 函数，打印模拟器的运行统计信息
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) { // 定义一个函数，用于模拟 CPU 的工作，参数是一个无符号的 64 位整数，表示要执行的指令数
  g_print_step = (n < MAX_INST_TO_PRINT); // 把全局变量 g_print_step，表示是否打印每条指令的信息，赋值为 n 是否小于 MAX_INST_TO_PRINT，表示最多打印的指令数
  switch (nemu_state.state) { // 用一个 switch 语句，根据模拟器的状态进行不同的处理
    case NEMU_END: case NEMU_ABORT: // 如果模拟器的状态是结束或中止
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n"); // 输出程序执行已经结束的信息，提示用户重新运行模拟器
      return; // 返回函数
    default: nemu_state.state = NEMU_RUNNING; // 如果模拟器的状态是其他情况，就把模拟器的状态设置为运行中
  }

  uint64_t timer_start = get_time(); // 定义一个局部变量，用于存放模拟器开始执行的时间，调用 get_time 函数获取当前的时间

  execute(n); // 调用 execute 函数，执行 n 条指令

  uint64_t timer_end = get_time(); // 定义一个局部变量，用于存放模拟器结束执行的时间，调用 get_time 函数获取当前的时间
  g_timer += timer_end - timer_start; // 把全局变量 g_timer，表示模拟器的运行时间，增加结束时间减去开始时间的差值

  switch (nemu_state.state) { // 用一个 switch 语句，根据模拟器的状态进行不同的处理
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break; // 如果模拟器的状态是运行中，就把模拟器的状态设置为停止

    case NEMU_END: case NEMU_ABORT: // 如果模拟器的状态是结束或中止
      Log("nemu: %s at pc = " FMT_WORD, // 输出模拟器的状态和停止的指令的地址到日志中
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) : // 如果模拟器的状态是中止，就用红色的字体输出 ABORT
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) : // 如果模拟器的状态是结束，并且返回值是 0，就用绿色的字体输出 HIT GOOD TRAP
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))), // 如果模拟器的状态是结束，并且返回值不是 0，就用红色的字体输出 HIT BAD TRAP
          nemu_state.halt_pc); // 输出停止的指令的地址
      // fall through // 注释表示这里没有 break，会继续执行下面的 case
    case NEMU_QUIT: statistic(); // 如果模拟器的状态是退出，就调用 statistic 函数，打印模拟器的运行统计信息
  }
}

