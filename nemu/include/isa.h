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

#ifndef __ISA_H__ // 如果没有定义 __ISA_H__ 这个宏，就执行下面的代码
#define __ISA_H__ // 定义 __ISA_H__ 这个宏，用于防止重复包含这个头文件

// Located at src/isa/$(GUEST_ISA)/include/isa-def.h
#include <isa-def.h> // 包含一个根据不同的 ISA 定义的头文件，例如 x86 或 mips32

// The macro `__GUEST_ISA__` is defined in $(CFLAGS).
// It will be expanded as "x86" or "mips32" ...
typedef concat(__GUEST_ISA__, _CPU_state) CPU_state; // 根据 __GUEST_ISA__ 宏的定义，给 CPU 状态结构体类型取一个别名，例如 x86_CPU_state 或 mips32_CPU_state
typedef concat(__GUEST_ISA__, _ISADecodeInfo) ISADecodeInfo; // 根据 __GUEST_ISA__ 宏的定义，给 ISA 解码信息结构体类型取一个别名，例如 x86_ISADecodeInfo 或 mips32_ISADecodeInfo

// monitor
extern unsigned char isa_logo[]; // 声明一个外部的无符号字符数组，用于存放 ISA 的 logo
void init_isa(); // 声明一个函数，用于初始化 ISA

// reg
extern CPU_state cpu; // 声明一个外部的 CPU 状态结构体变量，用于存放 CPU 的寄存器和程序计数器等信息
void isa_reg_display(); // 声明一个函数，用于显示 CPU 的寄存器的值
word_t isa_reg_str2val(const char *name, bool *success); // 声明一个函数，用于根据寄存器的名字，返回寄存器的值，如果成功，把 success 设为 true，否则设为 false

// exec
struct Decode; // 声明一个结构体类型，用于存放指令的解码信息
int isa_exec_once(struct Decode *s); // 声明一个函数，用于执行一条指令

// memory
enum { MMU_DIRECT, MMU_TRANSLATE, MMU_FAIL }; // 定义一个枚举类型，用于表示内存管理单元（MMU）的状态，分别是直接访问、地址转换和访问失败
enum { MEM_TYPE_IFETCH, MEM_TYPE_READ, MEM_TYPE_WRITE }; // 定义一个枚举类型，用于表示内存访问的类型，分别是指令取址、数据读取和数据写入
enum { MEM_RET_OK, MEM_RET_FAIL, MEM_RET_CROSS_PAGE }; // 定义一个枚举类型，用于表示内存访问的结果，分别是成功、失败和跨页访问
#ifndef isa_mmu_check // 如果没有定义 isa_mmu_check 这个宏，就执行下面的代码
int isa_mmu_check(vaddr_t vaddr, int len, int type); // 声明一个函数，用于检查虚拟地址的有效性，返回 MMU 的状态
#endif
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type); // 声明一个函数，用于把虚拟地址转换为物理地址，返回物理地址

// interrupt/exception
vaddr_t isa_raise_intr(word_t NO, vaddr_t epc); // 声明一个函数，用于处理中断或异常，返回异常处理程序的入口地址
#define INTR_EMPTY ((word_t)-1) // 定义一个宏，用于表示没有中断或异常发生
word_t isa_query_intr(); // 声明一个函数，用于查询是否有中断或异常发生，返回中断或异常的编号，如果没有，返回 INTR_EMPTY

// difftest
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc); // 声明一个函数，用于进行差分测试，比较当前 CPU 状态和参考 CPU 状态是否一致，返回 true 或 false
void isa_difftest_attach(); // 声明一个函数，用于进行差分测试，把当前 CPU 状态和参考 CPU 状态连接起来

#endif // 结束条件编译，与 #ifndef 对应

