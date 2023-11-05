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
#define macro(i) /* 定义一个宏，用于处理字符串中的第 i 个字符 */ \
  if ((i) >= len) goto finish; /* 如果 i 超过了字符串的长度，就跳转到 finish 标签 */ \
  else { /* 否则 */ \
    char c = str[i]; /* 定义一个字符变量，用于存放字符串中的第 i 个字符 */ \
    if (c != ' ') { /* 如果这个字符不是空格 */ \
      Assert(c == '0' || c == '1' || c == '?', /* 断言这个字符是 0 或 1 或 ?，否则报错 */ \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); /* 根据这个字符的值，更新关键字，如果是 1，就在最低位加 1，否则加 0 */ \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); /* 根据这个字符的值，更新掩码，如果是 ?，就在最低位加 0，否则加 1 */ \
      __shift = (c == '?' ? __shift + 1 : 0); /* 根据这个字符的值，更新位移，如果是 ?，就加 1，否则重置为 0 */ \
    } /* 结束 if */ \
  } /* 结束 else */

#define macro2(i)  macro(i);   macro((i) + 1) /* 定义一个宏，用于处理字符串中的第 i 和 i+1 个字符，调用 macro 宏 */
#define macro4(i)  macro2(i);  macro2((i) + 2) /* 定义一个宏，用于处理字符串中的第 i 到 i+3 个字符，调用 macro2 宏 */
#define macro8(i)  macro4(i);  macro4((i) + 4) /* 定义一个宏，用于处理字符串中的第 i 到 i+7 个字符，调用 macro4 宏 */
#define macro16(i) macro8(i);  macro8((i) + 8) /* 定义一个宏，用于处理字符串中的第 i 到 i+15 个字符，调用 macro8 宏 */
#define macro32(i) macro16(i); macro16((i) + 16) /* 定义一个宏，用于处理字符串中的第 i 到 i+31 个字符，调用 macro16 宏 */
#define macro64(i) macro32(i); macro32((i) + 32) /* 定义一个宏，用于处理字符串中的第 i 到 i+63 个字符，调用 macro32 宏 */
  macro64(0); /* 调用 macro64 宏，从第 0 个字符开始处理 */
  panic("pattern too long"); /* 如果字符串的长度超过了 64，就报错 */
#undef macro /* 取消 macro 宏的定义 */
finish: /* 定义一个标签，用于跳转 */
  *key = __key >> __shift; /* 把关键字右移位移位，赋值给指针指向的变量 */
  *mask = __mask >> __shift; /* 把掩码右移位移位，赋值给指针指向的变量 */
  *shift = __shift; /* 把位移赋值给指针指向的变量 */
} /* 结束函数 */

__attribute__((always_inline)) /* 这是一个属性，表示这个函数总是被内联，即在调用时用函数体替换函数名，以减少函数调用的开销 */
static inline void pattern_decode_hex(const char *str, int len, /* 定义一个静态内联函数，用于解码十六进制的指令模式，参数是一个字符串和它的长度 */
    uint64_t *key, uint64_t *mask, uint64_t *shift) { /* 还有三个指针，用于返回解码后的关键字、掩码和位移 */
  uint64_t __key = 0, __mask = 0, __shift = 0; /* 定义三个局部变量，用于存放解码过程中的关键字、掩码和位移 */
#define macro(i) /* 定义一个宏，用于处理字符串中的第 i 个字符 */ \
  if ((i) >= len) goto finish; /* 如果 i 超过了字符串的长度，就跳转到 finish 标签 */ \
  else { /* 否则 */ \
    char c = str[i]; /* 定义一个字符变量，用于存放字符串中的第 i 个字符 */ \
    if (c != ' ') { /* 如果这个字符不是空格 */ \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', /* 断言这个字符是 0 到 9 或 a 到 f 或 ?，否则报错 */ \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); /* 根据这个字符的值，更新关键字，如果是 ?，就在最低四位加 0，否则加上字符对应的十六进制数 */ \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); /* 根据这个字符的值，更新掩码，如果是 ?，就在最低四位加 0，否则加上 0xf */ \
      __shift = (c == '?' ? __shift + 4 : 0); /* 根据这个字符的值，更新位移，如果是 ?，就加 4，否则重置为 0 */ \
    } /* 结束 if */ \
  } /* 结束 else */

  macro16(0); /* 调用 macro16 宏，从第 0 个字符开始处理 */
  panic("pattern too long"); /* 如果字符串的长度超过了 16，就报错 */
#undef macro /* 取消 macro 宏的定义 */
finish: /* 定义一个标签，用于跳转 */
  *key = __key >> __shift; /* 把关键字右移位移位，赋值给指针指向的变量 */
  *mask = __mask >> __shift; /* 把掩码右移位移位，赋值给指针指向的变量 */
  *shift = __shift; /* 把位移赋值给指针指向的变量 */
}


// --- pattern matching wrappers for decode ---
#define INSTPAT(pattern, ...) do { /* 定义一个宏，用于根据指定的模式匹配指令，参数是一个模式字符串和可变参数 */ \
  uint64_t key, mask, shift; /* 定义三个变量，用于存放模式的关键字、掩码和位移 */ \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); /* 调用 pattern_decode 函数，用指针传递参数 */ \
  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) { /* 如果指令的相应位与模式的关键字匹配 */ \
    INSTPAT_MATCH(s, ##__VA_ARGS__); /* 调用 INSTPAT_MATCH 宏，传递可变参数 */ \
    goto *(__instpat_end); /* 跳转到结束标签 */ \
  } \
} while (0) /* 结束宏定义 */

#define INSTPAT_START(name) { const void ** __instpat_end = &&concat(__instpat_end_, name); /* 定义一个宏，用于开始一个模式匹配的区域，参数是一个名称，用于生成结束标签 */ 
#define INSTPAT_END(name)   concat(__instpat_end_, name): ; } /*定义一个宏，用于结束一个模式匹配的区域，参数是一个名称，用于生成结束标签*/
#endif
