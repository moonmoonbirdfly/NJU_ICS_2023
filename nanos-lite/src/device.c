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
  // 从GPU配置获取屏幕宽度和高度
  AM_GPU_CONFIG_T config = io_read(AM_GPU_CONFIG);
  int width = config.width;
  int height = config.height;

  // 计算开始绘制的位置和像素
  int x = offset % width; // 行内偏移
  int y = offset / width; // 行偏移
  if (y >= height) return 0; // 如果偏移超过屏幕尺寸，不进行任何绘制

  // 计算实际可以绘制的字节数
  size_t bytes_writeable = (len + offset > width * height * 4) ? 
                           (width * height * 4 - offset) : len;
  size_t pixels_writeable = bytes_writeable / 4; // 将字节转换为像素数

  // 使用io_write绘制到屏幕上
  io_write(AM_GPU_FBDRAW, x, y,(void*) buf, pixels_writeable, 1, true);

  return bytes_writeable; // 返回实际写入的字节数
}



void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
