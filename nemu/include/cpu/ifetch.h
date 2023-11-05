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

#ifndef __CPU_IFETCH_H__

#include <memory/vaddr.h>

static inline uint32_t inst_fetch(vaddr_t *pc, int len) { // 定义一个静态内联函数，用于从虚拟地址空间中取出指令，参数是一个虚拟地址指针和一个整数，表示指令的长度
  uint32_t inst = vaddr_ifetch(*pc, len); // 定义一个无符号的 32 位整数，用于存放指令的值，调用 vaddr_ifetch 函数，传递虚拟地址指针指向的地址和指令的长度，从虚拟地址空间中取出指令
  (*pc) += len; // 把虚拟地址指针指向的地址增加指令的长度，表示指向下一条指令的地址
  return inst; // 返回指令的值
}


#endif
