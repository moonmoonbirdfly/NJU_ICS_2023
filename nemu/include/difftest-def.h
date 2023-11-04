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

#ifndef __DIFFTEST_DEF_H__
#define __DIFFTEST_DEF_H__

#include <stdint.h> // 包含标准整型定义
#include <macro.h>  // 包含宏定义文件，假设定义了MUXDEF等宏
#include <generated/autoconf.h> // 包含生成的自动配置头文件

#define __EXPORT __attribute__((visibility("default"))) // 设置默认的符号可见性为导出

// 定义枚举，用于指明差异测试的方向，DIFFTEST_TO_DUT 表示测试数据发送到被测试设计，DIFFTEST_TO_REF 表示发送到参考模型
enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

#if defined(CONFIG_ISA_x86) // 如果定义了 CONFIG_ISA_x86，即x86架构
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 9) // x86架构的GPRs加上程序计数器pc的大小

#elif defined(CONFIG_ISA_mips32) // 如果定义了 CONFIG_ISA_mips32，即MIPS32架构
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 38) // MIPS32架构的GPRs加上状态寄存器status等的大小

#elif defined(CONFIG_ISA_riscv) // 如果定义了 CONFIG_ISA_riscv，即RISC-V架构
#define RISCV_GPR_TYPE MUXDEF(CONFIG_RV64, uint64_t, uint32_t) // 根据是否定义了 CONFIG_RV64，选择适合的通用寄存器类型
#define RISCV_GPR_NUM  MUXDEF(CONFIG_RVE , 16, 32) // 根据是否定义了 CONFIG_RVE，选择通用寄存器的数量
#define DIFFTEST_REG_SIZE (sizeof(RISCV_GPR_TYPE) * (RISCV_GPR_NUM + 1)) // RISC-V架构的GPRs加上程序计数器pc的大小

#elif defined(CONFIG_ISA_loongarch32r) // 如果定义了 CONFIG_ISA_loongarch32r，即龙芯32位R型架构
# define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 33) // 龙芯32位R型架构的GPRs加上程序计数器pc的大小

#else
# error Unsupport ISA // 如果没有支持的ISA定义，抛出错误
#endif

#endif // __DIFFTEST_DEF_H__



