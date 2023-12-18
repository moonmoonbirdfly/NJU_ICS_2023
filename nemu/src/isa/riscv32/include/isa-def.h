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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

typedef struct {
  word_t mcause;
  vaddr_t mepc;
  word_t mstatus;
  word_t mtvec;
} MUXDEF(CONFIG_RV64, riscv64_CSRs, riscv32_CSRs);
// 定义一个结构体，用于存储 RISC-V CPU 的状态
typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)]; // 通用寄存器数组，其大小根据 CONFIG_RVE 的值来确定（如果 CONFIG_RVE 为真，大小为16；否则为32）
  vaddr_t pc; // 程序计数器
  MUXDEF(CONFIG_RV64, riscv64_CSRs, riscv32_CSRs) csr;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// 定义一个结构体，用于存储指令解码相关的信息
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

// 定义一个宏，用于执行内存管理单元的检查，始终返回 MMU_DIRECT。
#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif

