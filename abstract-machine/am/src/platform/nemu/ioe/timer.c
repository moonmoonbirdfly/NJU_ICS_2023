#include <am.h>
#include <nemu.h>

// 初始化 timer，将 timer 先设置为 0
void __am_timer_init() {
  // 输出一串数据到 I/O 地址，这里是定时器的一个实现
  // outl 函数将 value 的值写入 port 指定的 I/O 地址
  outl(RTC_ADDR, 0);
  outl(RTC_ADDR + 4, 0);
}

// 获取从系统启动到现在的时间
void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 通过读取硬件 timer 的当前值，获取系统已经运行的时间
  // inl 函数从 port 指定的 I/O 地址读取一个 dword（32位值）
  uptime->us <<= 32;
  uptime->us = inl(RTC_ADDR + 4);
  // 由于 timer 的值可能会超过 32 位，所以这里需要读两次，然后将这两次的值合并成一个 64 位的整数
  
  uptime->us += inl(RTC_ADDR);
}

// 获取现实世界的时间，这里并没有实现，只是简单地将所有字段都设为了固定值
void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

