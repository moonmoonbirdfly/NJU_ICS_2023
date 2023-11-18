#include <am.h>

// 系统启动的时间点
static uint64_t boot_time = 0;

// 定义CLINT（Core Local Interruptor）的基地址，CLINT由RISC-V提供，包含了timer和software interrupt功能
#define CLINT_MMIO 0x2000000ul

// 指定一个地址偏移，用于读取 timer 的值
#define TIME_BASE 0xbff8

// 读取当前的时间，时间的单位是微秒(us)
static uint64_t read_time() {
  // 通过 MMIO 直接读取硬件 timer 的当前值。timer 的值是一个 64 位的整数，
  // 分别通过两次 32 位的读操作来获取，然后合并为一个 64 位的整数
  uint32_t lo = *(volatile uint32_t *)(CLINT_MMIO + TIME_BASE + 0);
  uint32_t hi = *(volatile uint32_t *)(CLINT_MMIO + TIME_BASE + 4);
  uint64_t time = ((uint64_t)hi << 32) | lo;
  // 由于实际得到的 timer 的值是以 1/10us 为单位的，所以这里需要除以10，得到以 us 为单位的时间
  return time / 10;
}

// 获取从系统启动至今的运行时间
void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 当前的时间减去系统启动的时间就是系统运行的时间
  uptime->us = read_time() - boot_time;
}

// 初始化 timer
void __am_timer_init() {
  // 准备记录系统启动的时间
  boot_time = read_time();
}

// 获取现实世界的时间，这里似乎并没有真正实现，只是简单地将所有字段都设为了0或1900
void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

