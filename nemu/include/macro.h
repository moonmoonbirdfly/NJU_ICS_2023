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

#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>

// macro stringizing
#define str_temp(x) #x
#define str(x) str_temp(x)

// strlen() for string constant
#define STRLEN(CONST_STR) (sizeof(CONST_STR) - 1)

// calculate the length of an array
#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))

// 宏连接
#define concat_temp(x, y) x ## y // 连接两个宏参数
#define concat(x, y) concat_temp(x, y) // 间接连接两个宏参数
#define concat3(x, y, z) concat(concat(x, y), z) // 连接三个宏参数
#define concat4(x, y, z, w) concat3(concat(x, y), z, w) // 连接四个宏参数
#define concat5(x, y, z, v, w) concat4(concat(x, y), z, v, w) // 连接五个宏参数

// macro testing
// See https://stackoverflow.com/questions/26099745/test-if-preprocessor-symbol-is-defined-inside-macro
#define CHOOSE2nd(a, b, ...) b // CHOOSE2nd宏用于选择传入参数的第二个参数。

#define MUX_WITH_COMMA(contain_comma, a, b) CHOOSE2nd(contain_comma a, b) // MUX_WITH_COMMA宏通过检查contain_comma是否包含逗号来决定是选择a还是b。

#define MUX_MACRO_PROPERTY(p, macro, a, b) MUX_WITH_COMMA(concat(p, macro), a, b) // MUX_MACRO_PROPERTY宏结合了宏属性和MUX_WITH_COMMA的选择机制。

// 定义一些属性的占位符
#define __P_DEF_0  X, // __P_DEF_0是一个占位宏，用于当宏没有定义时的情况。
#define __P_DEF_1  X, // __P_DEF_1是一个占位宏，用于当宏已定义时的情况。
#define __P_ONE_1  X, // __P_ONE_1是一个占位宏，用于特定宏的值为1时的情况。
#define __P_ZERO_0 X, // __P_ZERO_0是一个占位宏，用于特定宏的值为0时的情况。

// 根据BOOLEAN宏的属性定义一些选择函数
#define MUXDEF(macro, X, Y)  MUX_MACRO_PROPERTY(__P_DEF_, macro, X, Y) // MUXDEF用于选择当宏macro定义了时的值X，否则为Y。
#define MUXNDEF(macro, X, Y) MUX_MACRO_PROPERTY(__P_DEF_, macro, Y, X) // MUXNDEF用于选择当宏macro没有定义时的值X，否则为Y。
#define MUXONE(macro, X, Y)  MUX_MACRO_PROPERTY(__P_ONE_, macro, X, Y) // MUXONE用于选择当宏macro的值为1时的值X，否则为Y。
#define MUXZERO(macro, X, Y) MUX_MACRO_PROPERTY(__P_ZERO_, macro, X, Y) // MUXZERO用于选择当宏macro的值为0时的值X，否则为Y。


// test if a boolean macro is defined
#define ISDEF(macro) MUXDEF(macro, 1, 0)
// test if a boolean macro is undefined
#define ISNDEF(macro) MUXNDEF(macro, 1, 0)
// test if a boolean macro is defined to 1
#define ISONE(macro) MUXONE(macro, 1, 0)
// test if a boolean macro is defined to 0
#define ISZERO(macro) MUXZERO(macro, 1, 0)
// test if a macro of ANY type is defined
// NOTE1: it ONLY works inside a function, since it calls `strcmp()`
// NOTE2: macros defined to themselves (#define A A) will get wrong results
#define isdef(macro) (strcmp("" #macro, "" str(macro)) != 0)

// 宏定义条件编译简化
#define __IGNORE(...) // 忽略参数
#define __KEEP(...) __VA_ARGS__ // 保持参数
// 如果宏定义了，就保留代码
#define IFDEF(macro, ...) MUXDEF(macro, __KEEP, __IGNORE)(__VA_ARGS__)
// 如果宏没定义，就保留代码
#define IFNDEF(macro, ...) MUXNDEF(macro, __KEEP, __IGNORE)(__VA_ARGS__)
// 如果宏定义为1，就保留代码
#define IFONE(macro, ...) MUXONE(macro, __KEEP, __IGNORE)(__VA_ARGS__)
// 如果宏定义为0，就保留代码
#define IFZERO(macro, ...) MUXZERO(macro, __KEEP, __IGNORE)(__VA_ARGS__)

// functional-programming-like macro (X-macro)
// apply the function `f` to each element in the container `c`
// NOTE1: `c` should be defined as a list like:
//   f(a0) f(a1) f(a2) ...
// NOTE2: each element in the container can be a tuple
// 函数式编程风格的宏（X-macro）
// 将函数`f`应用于容器`c`中的每个元素
#define MAP(c, f) c(f)

#define BITMASK(bits) ((1ull << (bits)) - 1) // 创建位掩码
#define BITS(x, hi, lo) (((x) >> (lo)) & BITMASK((hi) - (lo) + 1)) // 提取x的位段(hi --> lo)
#define SEXT(x, len) ({ struct { int64_t n : len; } __x = { .n = x }; (uint64_t)__x.n; }) // 符号扩展

#define ROUNDUP(a, sz)   ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1)) // 向上取整到sz的倍数
#define ROUNDDOWN(a, sz) ((((uintptr_t)a)) & ~((sz) - 1)) // 向下取整到sz的倍数

#define PG_ALIGN __attribute((aligned(4096))) // 页面对齐

#if !defined(likely)
#define likely(cond)   __builtin_expect(cond, 1) // 条件表达式可能为真
#define unlikely(cond) __builtin_expect(cond, 0) // 条件表达式可能为假
#endif

// for AM IOE
#define io_read(reg) \
  ({ reg##_T __io_param; \
    ioe_read(reg, &__io_param); \
    __io_param; })

#define io_write(reg, ...) \
  ({ reg##_T __io_param = (reg##_T) { __VA_ARGS__ }; \
    ioe_write(reg, &__io_param); })

#endif
