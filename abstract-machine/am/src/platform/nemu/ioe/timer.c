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
    uint32_t lo = inl(RTC_LO_ADDR);
    uint32_t hi = inl(RTC_HI_ADDR);
    // 注意这里是把高位数值左移32位，与低位做或操作，得到64位的时间值
    *uptime = ((uint64_t)hi << 32) | lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

