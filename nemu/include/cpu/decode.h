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

#ifndef __CPU_DECODE_H__ // 如果没有定义 __CPU_DECODE_H__ 这个宏，就执行下面的代码
#define __CPU_DECODE_H__ // 定义 __CPU_DECODE_H__ 这个宏，用于防止重复包含这个头文件

#include <isa.h> // 包含一个根据不同的 ISA 定义的头文件，例如 x86 或 mips32

typedef struct Decode { // 定义一个结构体类型，用于存放指令的解码信息
  vaddr_t pc; // 定义一个变量，用于存放指令的地址
  vaddr_t snpc; // static next pc // 定义一个变量，用于存放静态的下一条指令的地址
  vaddr_t dnpc; // dynamic next pc // 定义一个变量，用于存放动态的下一条指令的地址
  ISADecodeInfo isa; // 定义一个变量，用于存放 ISA 的解码信息，例如指令的原始值或分解后的字段
  IFDEF(CONFIG_ITRACE, char logbuf[128]); // 如果定义了 CONFIG_ITRACE 宏，就定义一个字符数组，用于存放指令的日志信息
} Decode; // 给这个结构体类型取一个名字，叫 Decode

// --- pattern matching mechanism --- // 这是一个注释，表示下面的代码是用于实现指令的模式匹配的机制
__attribute__((always_inline)) // 这是一个属性，表示这个函数总是被内联，即在调用时用函数体替换函数名，以减少函数调用的开销
static inline void pattern_decode(const char *str, int len, // 定义一个静态内联函数，用于解码指令的模式，参数是一个字符串和它的长度
    uint64_t *key, uint64_t *mask, uint64_t *shift) { // 还有三个指针，用于返回解码后的关键字、掩码和位移
  uint64_t __key = 0, __mask = 0, __shift = 0; // 定义三个局部变量，用于存放解码过程中的关键字、掩码和位移
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }

#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

__attribute__((always_inline))
static inline void pattern_decode_hex(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); \
      __shift = (c == '?' ? __shift + 4 : 0); \
    } \
  }

  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}


// --- pattern matching wrappers for decode ---
#define INSTPAT(pattern, ...) do { \
  uint64_t key, mask, shift; \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) { \
    INSTPAT_MATCH(s, ##__VA_ARGS__); \
    goto *(__instpat_end); \
  } \
} while (0)

#define INSTPAT_START(name) { const void ** __instpat_end = &&concat(__instpat_end_, name);
#define INSTPAT_END(name)   concat(__instpat_end_, name): ; }

#endif
