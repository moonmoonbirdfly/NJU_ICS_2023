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

#ifndef __DEVICE_MAP_H__   //防止头文件重复包含
#define __DEVICE_MAP_H__

#include <cpu/difftest.h>  //包含cpu的差异测试头文件

// 定义I/O操作的回调函数的类型
typedef void(*io_callback_t)(uint32_t, int, bool);

// 定义一个函数，用于申请大小为size的新空间
uint8_t* new_space(int size);

// 定义I/O映射的结构体
typedef struct {
  const char *name;         // 设备名
  paddr_t low;              // 映射的起始地址
  paddr_t high;             // 映射的结束地址
  void *space;              // 映射的地址空间
  io_callback_t callback;   // I/O操作时的回调函数
} IOMap;

// 检查指定地址是否在映射范围内的内联函数
static inline bool map_inside(IOMap *map, paddr_t addr) {
  return (addr >= map->low && addr <= map->high);
}

// 根据指定地址找到相关的映射ID的内联函数
static inline int find_mapid_by_addr(IOMap *maps, int size, paddr_t addr) {
  int i;
  // 遍历所有映射
  for (i = 0; i < size; i ++) {
    // 如果地址在当前映射范围内，跳过参考测试并返回当前映射ID
    if (map_inside(maps + i, addr)) {
      difftest_skip_ref();
      return i;
    }
  }
  // 如果没有找到相关的映射，返回-1
  return -1;
}

// 添加PIO映射的函数
void add_pio_map(const char *name, ioaddr_t addr,
        void *space, uint32_t len, io_callback_t callback);
// 添加MMIO映射的函数
void add_mmio_map(const char *name, paddr_t addr,
        void *space, uint32_t len, io_callback_t callback);

// 基于地址和数据长度从特定的I/O映射中读取的函数
word_t map_read(paddr_t addr, int len, IOMap *map);
// 基于地址、数据长度和实际数据内容写入特定的I/O映射的函数
void map_write(paddr_t addr, int len, word_t data, IOMap *map);

#endif

