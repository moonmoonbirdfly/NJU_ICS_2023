#include <common.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; ++i) putch(*((char *)buf + i));
  return len;
}
size_t events_read(void *buf, size_t offset, size_t len) {
    AM_INPUT_KEYBRD_T t = io_read(AM_INPUT_KEYBRD);
    if (t.keycode == AM_KEY_NONE) {
    *(char*)buf = '\0';
    return 0;
  }
   return snprintf((char *)buf, len, "%s %s\n", 
    t.keydown ? "kd" : "ku",
    keyname[t.keycode]);
}


size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T t = io_read(AM_GPU_CONFIG);
  return snprintf((char *)buf, len, "WIDTH : %d\nHEIGHT : %d", t.width, t.height);
}


//buf中的len字节写到屏幕上offset处
size_t fb_write(const void *buf, size_t offset, size_t len) {

  // const uint32_t *src = (uint32_t *)buf;
  // uint32_t *fb = (uint32_t *)(uintptr_t)(FB_ADDR + offset); //字节编址

  // for (int i = 0; i < len / 4; ++i){
  //   fb[i] = src[i];
  // }
  //yield();
  uintptr_t *ptr;
  ptr = (uintptr_t *)(&buf);

  io_write(AM_GPU_MEMCPY, offset, (void *)*ptr, len);
  io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true);
  
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
