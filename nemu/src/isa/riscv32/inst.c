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

#include "local-include/reg.h" // 包含一些寄存器的定义和操作
#include <cpu/cpu.h> // 包含 CPU 的结构和函数
#include <cpu/ifetch.h> // 包含指令取址的函数
#include <cpu/decode.h> // 包含指令解码的函数

#define R(i) gpr(i) // 定义一个宏，用于访问通用寄存器的值
#define Mr vaddr_read // 定义一个宏，用于读取虚拟地址的内容
#define Mw vaddr_write // 定义一个宏，用于写入虚拟地址的内容

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_R, TYPE_B, TYPE_J,
  TYPE_N, // none
}; // 定义一个枚举类型，用于表示不同的指令格式

#define src1R() do { *src1 = R(rs1); } while (0) // 定义一个宏，用于获取源寄存器 1 的值
#define src2R() do { *src2 = R(rs2); } while (0) // 定义一个宏，用于获取源寄存器 2 的值
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0) // 定义一个宏，用于获取 I 型指令的立即数
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0) // 定义一个宏，用于获取 U 型指令的立即数
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0) // 定义一个宏，用于获取 S 型指令的立即数
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | (BITS(i, 7, 7) << 11) | (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1); } while(0)//// 定义一个宏，用于获取 B 型指令的立即数
// 定义一个宏，用于获取 J 型指令的立即数
#define immJ() do { *imm = SEXT(( (BITS(i, 31, 31) << 19) | BITS(i, 30, 21) | (BITS(i, 20, 20) << 10) | (BITS(i, 19, 12) << 11) ) << 1, 21);  } while(0)
// 定义一个宏，用于获取 R 型指令的功能码
#define func3() BITS(i, 14, 12)
// 定义一个宏，用于获取 R 型指令的功能码
#define func7() BITS(i, 31, 25)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  // 定义一个静态函数，用于解码指令的操作数
  uint32_t i = s->isa.inst.val; // 获取指令的原始值
  int rs1 = BITS(i, 19, 15); // 获取源寄存器 1 的编号
  int rs2 = BITS(i, 24, 20); // 获取源寄存器 2 的编号
  *rd     = BITS(i, 11, 7); // 获取目标寄存器的编号
  switch (type) { // 根据指令的格式，获取不同的操作数
    case TYPE_I: src1R();          immI(); break; // I 型指令，获取源寄存器 1 和立即数
    case TYPE_U:                   immU(); break; // U 型指令，获取立即数
    case TYPE_S: src1R(); src2R(); immS(); break; // S 型指令，获取源寄存器 1、源寄存器 2 和立即数
    case TYPE_R: src1R(); src2R(); 	   break; // R 型指令，获取源寄存器 1 和源寄存器 2
    case TYPE_B: src1R(); src2R(); immB(); break; // B 型指令，获取源寄存器 1、源寄存器 2 和立即数
    case TYPE_J:                   immJ(); break; // J 型指令，获取立即数
  }
}

static int decode_exec(Decode *s) {
  // 定义一个静态函数，用于解码和执行指令
  int rd = 0; // 定义一个变量，用于存放目标寄存器的编号
  word_t src1 = 0, src2 = 0, imm = 0; // 定义三个变量，用于存放操作数的值
  s->dnpc = s->snpc; // 设置下一条指令的地址

#define INSTPAT_INST(s) ((s)->isa.inst.val) // 定义一个宏，用于获取指令的原始值
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
} // 定义一个宏，用于匹配指令的名称和格式，并执行相应的操作

  INSTPAT_START(); // 定义一个宏，用于开始匹配指令
  
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, s->dnpc = s->pc; s->dnpc += imm; R(rd) = s->pc + 4);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, s->dnpc = (src1 + imm) & ~(word_t)1; R(rd) = s->pc + 4);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));

  INSTPAT_END(); // 定义一个宏，用于结束匹配指令

  R(0) = 0; // reset $zero to 0 // 把寄存器 0 的值重置为 0

  return 0; // 返回 0
}

int isa_exec_once(Decode *s) {
  // 定义一个函数，用于执行一条指令
  s->isa.inst.val = inst_fetch(&s->snpc, 4); // 从内存中取出一条指令，长度为 4 字节
  return decode_exec(s); // 调用 decode_exec 函数，解码和执行指令
}

