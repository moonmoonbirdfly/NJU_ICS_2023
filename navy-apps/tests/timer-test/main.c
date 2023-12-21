#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include<NDL.h>
/*
int main() {
  struct timeval tv;
  // struct timezone tz;
  gettimeofday(&tv, NULL);
  __uint64_t ms = 0;
  while (1) {
    while ((tv.tv_sec * 1000 + tv.tv_usec / 1000) < ms) {
      gettimeofday(&tv, NULL);
    }
    ms += 1000;
    printf("time = %d s\n", (int)ms/1000);
  }
}
*/

int main() {
  uint32_t last_tick = NDL_GetTicks();
  while (1) {
    uint32_t tick = NDL_GetTicks();
    if (tick - last_tick >= 500) {
      printf("time=%d s\n",(int)tick/1000);
      last_tick = tick;
    }
  }
  return 0;
}
