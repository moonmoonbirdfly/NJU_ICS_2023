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
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
	for(int i = 0; i< 32 ;i++){
			 printf("register #%s --> %x\n",regs[i],cpu.gpr[i]);
	}
}

word_t isa_reg_str2val(const char *s, bool *success) {
  // 定义一个函数，用于根据寄存器的名称返回其当前值，参数是一个字符串指针和一个布尔指针
  for (int i = 0; i < 32; i++) { // 用一个循环，遍历 32 个寄存器的名称
    if (strcmp(s, regs[i]) == 0) { // 如果字符串 s 和第 i 个寄存器的名称相等，说明找到了对应的寄存器
      *success = true; // 设置 success 指针指向的变量的值为 true，表示找到了寄存器
      return cpu.gpr[i]; // 返回 cpu 结构体中的第 i 个寄存器的值
    }
  }
  // 如果循环结束后没有找到对应的寄存器，说明 s 不是一个有效的寄存器名称
  *success = false; // 设置 success 指针指向的变量的值为 false，表示没有找到寄存器
  return 0; // 返回 0，表示无效的值
}



