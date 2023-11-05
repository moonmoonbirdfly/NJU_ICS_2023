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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC) // 如果定义了 CONFIG_PMEM_MALLOC 这个宏，表示使用动态分配的方式管理物理内存
static uint8_t *pmem = NULL; // 定义一个静态的字节指针，用于指向物理内存的起始地址，初始为 NULL
#else // CONFIG_PMEM_GARRAY // 否则，表示使用静态数组的方式管理物理内存
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {}; // 定义一个静态的字节数组，用于存放物理内存的内容，大小为 CONFIG_MSIZE，表示物理内存的大小，对齐为 PG_ALIGN，表示页对齐
#endif

uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; } // 定义一个函数，用于把物理地址转换为主机地址，参数是一个物理地址，返回值是一个字节指针，计算方法是物理内存的起始地址加上物理地址减去 CONFIG_MBASE，表示物理内存的基址
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; } // 定义一个函数，用于把主机地址转换为物理地址，参数是一个字节指针，返回值是一个物理地址，计算方法是主机地址减去物理内存的起始地址加上 CONFIG_MBASE，表示物理内存的基址

static word_t pmem_read(paddr_t addr, int len) { // 定义一个静态函数，用于从物理内存中读取数据，参数是一个物理地址和一个整数，表示读取的长度
  word_t ret = host_read(guest_to_host(addr), len); // 定义一个无符号的 64 位整数，用于存放读取的数据，调用 host_read 函数，传递主机地址和读取的长度，从主机内存中读取数据
  return ret; // 返回读取的数据
}

static void pmem_write(paddr_t addr, int len, word_t data) { // 定义一个静态函数，用于向物理内存中写入数据，参数是一个物理地址，一个整数，表示写入的长度，和一个无符号的 64 位整数，表示写入的数据
  host_write(guest_to_host(addr), len, data); // 调用 host_write 函数，传递主机地址，写入的长度和写入的数据，向主机内存中写入数据
}

static void out_of_bound(paddr_t addr) { // 定义一个静态函数，用于处理物理地址越界的情况，参数是一个物理地址
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD, // 调用 panic 函数，输出错误信息，包括物理地址，物理内存的范围，和 CPU 的 pc 寄存器的值
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() { // 定义一个函数，用于初始化物理内存
#if   defined(CONFIG_PMEM_MALLOC) // 如果定义了 CONFIG_PMEM_MALLOC 这个宏，表示使用动态分配的方式管理物理内存
  pmem = malloc(CONFIG_MSIZE); // 调用 malloc 函数，分配 CONFIG_MSIZE 大小的内存空间，把返回的指针赋值给 pmem
  assert(pmem); // 调用 assert 函数，断言 pmem 不为 NULL，否则报错
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE)); // 如果定义了 CONFIG_MEM_RANDOM 这个宏，表示使用随机数填充物理内存，就调用 memset 函数，传递物理内存的起始地址，随机数，和物理内存的大小，把随机数复制到物理内存中
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT); // 调用 Log 函数，输出物理内存的范围到日志中
}

word_t paddr_read(paddr_t addr, int len) { // 定义一个函数，用于从物理地址空间中读取数据，参数是一个物理地址和一个整数，表示读取的长度
  if (likely(in_pmem(addr))) return pmem_read(addr, len); // 如果物理地址在物理内存的范围内，就调用 pmem_read 函数，传递物理地址和读取的长度，从物理内存中读取数据，返回读取的数据
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len)); // 如果定义了 CONFIG_DEVICE 这个宏，表示开启了设备模拟，就调用 mmio_read 函数，传递物理地址和读取的长度，从设备的内存映射中读取数据，返回读取的数据
  out_of_bound(addr); // 如果物理地址既不在物理内存的范围内，也不在设备的内存映射中，就调用 out_of_bound 函数，处理物理地址越界的情况
  return 0; // 返回 0，表示无效的数据
}

void paddr_write(paddr_t addr, int len, word_t data) { // 定义一个函数，用于向物理地址空间中写入数据，参数是一个物理地址，一个整数，表示写入的长度，和一个无符号的 64 位整数，表示写入的数据
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; } // 如果物理地址在物理内存的范围内，就调用 pmem_write 函数，传递物理地址，写入的长度和写入的数据，向物理内存中写入数据，返回函数
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return); // 如果定义了 CONFIG_DEVICE 这个宏，表示开启了设备模拟，就调用 mmio_write 函数，传递物理地址，写入的长度和写入的数据，向设备的内存映射中写入数据，返回函数
  out_of_bound(addr); // 如果物理地址既不在物理内存的范围内，也不在设备的内存映射中，就调用 out_of_bound 函数，处理物理地址越界的情况
}

