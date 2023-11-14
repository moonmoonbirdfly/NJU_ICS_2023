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
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 19, 12) << 12) | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1); } while(0)
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
  
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm); // 匹配 lui 指令，执行 R(rd) = imm 的操作
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm); // 匹配 auipc 指令，执行 R(rd) = s->pc + imm 的操作
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1)); // 匹配 lbu 指令，执行 R(rd) = Mr(src1 + imm, 1) 的操作
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2)); // 匹配 lhu 指令，执行 R(rd) = Mr(src1 + imm, 2) 的操作
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8)); // 匹配 lb 指令，执行 R(rd) = SEXT(Mr(src1 + imm, 1), 8) 的操作
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16)); // 匹配 lh 指令，执行 R(rd) = SEXT(Mr(src1 + imm, 2), 16) 的操作
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4)); // 匹配 lw 指令，执行 R(rd) = Mr(src1 + imm, 4) 的操作
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2)); // 匹配 sb 指令，执行 Mw(src1 + imm, 1, src2) 的操作
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2)); // 匹配 sh 指令，执行 Mw(src1 + imm, 2, src2) 的操作
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2)); // 匹配 sw 指令，执行 Mw(src1 + imm, 4, src2) 的操作
  INSTPAT("0100000 ????? ????? 000 ????? 10001 11", srai   , I, R(rd) = (sword_t)src1 >> (imm & 0x1f)); // 匹配 srai 指令，执行 R(rd) = (sword_t)src1 >> (imm & 0x1f) 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 10100 11", srli   , I, R(rd) = src1 >> (imm & 0x1f)); // 匹配 srli 指令，执行 R(rd) = src1 >> (imm & 0x1f) 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm); // 匹配 addi 指令，执行 R(rd) = src1 + imm 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm); // 匹配 xori 指令，执行 R(rd) = src1 ^ imm 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 11000 11", slli   , I, R(rd) = src1 << (imm & 0x1f)); // 匹配 slli 指令，执行 R(rd) = src1 << (imm & 0x1f) 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 01000 11", slti   , I, R(rd) = (sword_t)src1 < (sword_t)imm); // 匹配 slti 指令，执行 R(rd) = (sword_t)src1 < (sword_t)imm 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 01001 11", sltiu  , I, R(rd) = src1 < imm); // 匹配 sltiu 指令，执行 R(rd) = src1 < imm 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 10000 11", andi   , I, R(rd) = src1 & imm); // 匹配 andi 指令，执行 R(rd) = src1 & imm 的操作	
  INSTPAT("0000000 ????? ????? 000 ????? 10001 11", ori    , I, R(rd) = src1 | imm); // 匹配 ori 指令，执行 R(rd) = src1 | imm 的操作
  INSTPAT("0100000 ????? ????? 001 ????? 11000 11", srl    , R, R(rd) = src1 >> (src2 & 0x1f)); // 匹配 srl 指令，执行 R(rd) = src1 >> (src2 & 0x1f) 的操作
  INSTPAT("0100000 ????? ????? 101 ????? 11000 11", sra    , R, R(rd) = (sword_t)src1 >> (src2 & 0x1f)); // 匹配 sra 指令，执行 R(rd) = (sword_t)src1 >> (src2 & 0x1f) 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 11000 11", add    , R, R(rd) = src1 + src2); // 匹配 add 指令，执行 R(rd) = src1 + src2 的操作
  INSTPAT("0000000 ????? ????? 001 ????? 11000 11", sll    , R, R(rd) = src1 << (src2 & 0x1f)); // 匹配 sll 指令，执行 R(rd) = src1 << (src2 & 0x1f) 的操作
  INSTPAT("0000000 ????? ????? 010 ????? 11000 11", slt    , R, R(rd) = (sword_t)src1 < (sword_t)src2); // 匹配 slt 指令，执行 R(rd) = (sword_t)src1 < (sword_t)src2 的操作
  INSTPAT("0000000 ????? ????? 011 ????? 11000 11", sltu   , R, R(rd) = src1 < src2); // 匹配 sltu 指令，执行 R(rd) = src1 < src2 的操作
  INSTPAT("0000000 ????? ????? 100 ????? 11000 11", xor    , R, R(rd) = src1 ^ src2); // 匹配 xor 指令，执行 R(rd) = src1 ^ src2 的操作
  INSTPAT("0000000 ????? ????? 101 ????? 11000 11", or     , R, R(rd) = src1 | src2); // 匹配 or 指令，执行 R(rd) = src1 | src2 的操作
  INSTPAT("0000000 ????? ????? 110 ????? 11000 11", and    , R, R(rd) = src1 & src2); // 匹配 and 指令，执行 R(rd) = src1 & src2 的操作
  INSTPAT("0100000 ????? ????? 000 ????? 11000 11", sub    , R, R(rd) = src1 - src2); // 匹配 sub 指令，执行 R(rd) = src1 - src2 的操作
  INSTPAT("0010000 ????? ????? 000 ????? 11000 11", addiw  , I, R(rd) = SEXT(src1 + imm, 32)); // 匹配 addiw 指令，执行 R(rd) = SEXT(src1 + imm, 32) 的操作
  INSTPAT("0010000 ????? ????? 001 ????? 11000 11", slliw  , I, R(rd) = SEXT(src1 << (imm & 0x1f), 32)); // 匹配 slliw 指令，执行 R(rd) = SEXT(src1 << (imm & 0x1f), 32) 的操作
  INSTPAT("0010000 ????? ????? 101 ????? 11000 11", srliw  , I, R(rd) = SEXT((uint32_t)src1 >> (imm & 0x1f), 32)); // 匹配 srliw 指令，执行 R(rd) = SEXT((uint32_t)src1 >> (imm & 0x1f), 32) 的操作
  INSTPAT("0110000 ????? ????? 101 ????? 11000 11", sraiw  , I, R(rd) = SEXT((int32_t)src1 >> (imm & 0x1f), 32)); // 匹配 sraiw 指令，执行 R(rd) = SEXT((int32_t)src1 >> (imm & 0x1f), 32) 的操作
  INSTPAT("0110000 ????? ????? 000 ????? 11000 11", addw   , R, R(rd) = SEXT(src1 + src2, 32)); // 匹配 addw 指令，执行 R(rd) = SEXT(src1 + src2, 32) 的操作
  INSTPAT("0110000 ????? ????? 001 ????? 11000 11", sllw   , R, R(rd) = SEXT(src1 << (src2 & 0x1f), 32)); // 匹配 sllw 指令，执行 R(rd) = SEXT(src1 << (src2 & 0x1f), 32) 的操作
  INSTPAT("0110000 ????? ????? 101 ????? 11000 11", srlw   , R, R(rd) = SEXT((uint32_t)src1 >> (src2 & 0x1f), 32));// 匹配 srlw 指令，执行 R(rd) = SEXT((uint32_t)src1 >> (src2 & 0x1f), 32)) 的操作
  INSTPAT("0000000 ????? ????? 000 ????? 11000 11", jalr   , I, R(rd) = s->snpc; s->dnpc = (src1 + imm) & ~1); // 匹配 jalr 指令，执行 R(rd) = s->snpc; s->dnpc = (src1 + imm) & ~1) 的操作
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jal    , J, R(rd) = s->snpc; s->dnpc = s->pc + imm); // 匹配 jal 指令，执行 R(rd) = s->snpc; s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? ??? ????? 11000 11", beq    , B, if (src1 == src2) s->dnpc = s->pc + imm); // 匹配 beq 指令，执行 if (src1 == src2) s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? ??? ????? 11001 11", bne    , B, if (src1 != src2) s->dnpc = s->pc + imm); // 匹配 bne 指令，执行 if (src1 != src2) s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? ??? ????? 11100 11", blt    , B, if ((sword_t)src1 < (sword_t)src2) s->dnpc = s->pc + imm); // 匹配 blt 指令，执行 if ((sword_t)src1 < (sword_t)src2) s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? ??? ????? 11101 11", bge    , B, if ((sword_t)src1 >= (sword_t)src2) s->dnpc = s->pc + imm); // 匹配 bge 指令，执行 if ((sword_t)src1 >= (sword_t)src2) s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? ??? ????? 11110 11", bltu   , B, if (src1 < src2) s->dnpc = s->pc + imm); // 匹配 bltu 指令，执行 if (src1 < src2) s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? ??? ????? 11111 11", bgeu   , B, if (src1 >= src2) s->dnpc = s->pc + imm); // 匹配 bgeu 指令，执行 if (src1 >= src2) s->dnpc = s->pc + imm) 的操作
  INSTPAT("??????? ????? ????? 110 ????? 00000 11", lwu    , I, R(rd) = SEXT(Mr(src1 + imm, 4), 32)); // 匹配 lwu 指令，执行 R(rd) = SEXT(Mr(src1 + imm, 4), 32) 的操作
  INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld     , I, R(rd) = Mr(src1 + imm, 8)); // 匹配 ld 指令，执行 R(rd) = Mr(src1 + imm, 8) 的操作
  INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd     , S, Mw(src1 + imm, 8, src2)); // 匹配 sd 指令，执行 Mw(src1 + imm, 8, src2) 的操作
  INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw   , R, R(rd) = SEXT(src1 - src2, 32)); // 匹配 subw 指令，执行 R(rd) = SEXT(src1 - src2, 32) 的操作
  INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw   , I, R(rd) = SEXT((sword_t)src1 >> (imm & 0x1f), 32)); // 匹配 sraw 指令，执行 R(rd) = SEXT((sword_t)src1 >> (imm & 0x1f), 32) 的操作
  
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2); // 匹配 mul 指令，执行 R(rd) = src1 * src2 的操作 
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (long long)src1 * (long long)src2 >> 32); // 匹配 mulh 指令，执行 R(rd) = (sword_t)src1 * (sword_t)src2 >> 32 的操作 
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R, R(rd) = (uint64_t)src1 * (long long)src2 >> 32); // 匹配 mulhsu 指令，执行 R(rd) = (sword_t)src1 * src2 >> 32 的操作 
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = (uint64_t)src1 * (uint64_t)src2 >> 32); // 匹配 mulhu 指令，执行 R(rd) = src1 * src2 >> 32 的操作 
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = src1 / src2); // 匹配 div 指令，执行 R(rd) = src1 / src2 的操作 
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = (word_t)src1 / (word_t)src2); // 匹配 divu 指令，执行 R(rd) = (word_t)src1 / (word_t)src2 的操作 
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = src1 % src2); // 匹配 rem 指令，执行 R(rd) = src1 % src2 的操作 
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = (word_t)src1 % (word_t)src2); // 匹配 remu 指令，执行 R(rd) = (word_t)src1 % (word_t)src2 的操作
 //INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall, N, ECALL(s->pc)); // 匹配 ecall 指令，执行 ECALL(s->pc) 的操作，其中 ECALL 是你需要实现的环境调用处理函数
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0 // 匹配 ebreak 指令，执行 NEMUTRAP(s->pc, R(10)) 的操作，其中 R(10) 是 $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc)); // 匹配无效指令，执行 INV(s->pc) 的操作
  INSTPAT_END(); // 定义一个宏，用于结束匹配指令

  R(0) = 0; // reset $zero to 0 // 把寄存器 0 的值重置为 0

  return 0; // 返回 0
}

int isa_exec_once(Decode *s) {
  // 定义一个函数，用于执行一条指令
  s->isa.inst.val = inst_fetch(&s->snpc, 4); // 从内存中取出一条指令，长度为 4 字节
  return decode_exec(s); // 调用 decode_exec 函数，解码和执行指令
}

