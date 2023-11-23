#include <am.h>
#include <nemu.h>
#include <klib.h>

#define RTC_LO_ADDR RTC_ADDR
#define RTC_HI_ADDR (RTC_ADDR+4)

// 这里假设0x48寄存器保存的时间单位是微秒
void __am_timer_init() {
   outl(RTC_LO_ADDR, 0);
   outl(RTC_HI_ADDR, 0);
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
      //uptime->us = 0;
     uptime->us=inl(RTC_ADDR+4);
      uptime->us<<=32;
      uptime->us+=inl(RTC_ADDR);
    } 

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

