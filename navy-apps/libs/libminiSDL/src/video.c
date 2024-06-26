#include <NDL.h>
#include <sdl-video.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static inline uint32_t translate_color(SDL_Color *color){
  return (color->a << 24) | (color->r << 16) | (color->g << 8) | color->b;
}
void SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(dst && src);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);

  // 计算源矩形的位置和尺寸
  int src_x = srcrect ? srcrect->x : 0;
  int src_y = srcrect ? srcrect->y : 0;
  int rect_w = srcrect ? srcrect->w : src->w;
  int rect_h = srcrect ? srcrect->h : src->h;

  // 计算目标矩形的位置
  int dst_x = dstrect ? dstrect->x : 0;
  int dst_y = dstrect ? dstrect->y : 0;

  // 检查像素位数
  if (src->format->BitsPerPixel != 8 && src->format->BitsPerPixel != 32) {
    assert(0); // 仅支持8位和32位像素格式
  }

  // 定义获得像素地址的宏
#define PIXEL_POS(surface, x, y) ((uint8_t*)(surface)->pixels + (y) * (surface)->pitch + (x) * ((surface)->format->BytesPerPixel))

  // 按照像素深度进行循环，复制对应的像素数据
  int bpp = src->format->BytesPerPixel;
  for (int i = 0; i < rect_h; ++i) {
    for (int j = 0; j < rect_w; ++j) {
      // 计算源与目标的像素位置
      uint8_t* src_pixel_pos = PIXEL_POS(src, src_x + j, src_y + i);
      uint8_t* dst_pixel_pos = PIXEL_POS(dst, dst_x + j, dst_y + i);

      // 根据像素位数进行像素数据复制
      if (bpp == 4) {
        *(uint32_t*)dst_pixel_pos = *(uint32_t*)src_pixel_pos;
      } else if (bpp == 1) {
        *dst_pixel_pos = *src_pixel_pos;
      }
    }
  }

#undef PIXEL_POS
}

void SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color) {
  // 确定要填充的区域
  int x, y, w, h;
  if (dstrect) {
    // 确保矩形区域不超出曲面的边界
    x = dstrect->x;
    y = dstrect->y;
    w = (x + dstrect->w > dst->w) ? dst->w - x : dstrect->w;
    h = (y + dstrect->h > dst->h) ? dst->h - y : dstrect->h;
  } else {
    x = y = 0;
    w = dst->w;
    h = dst->h;
  }

  // 计算开始填充的地址
  uint8_t *pixel = (uint8_t*)dst->pixels + y * dst->pitch + x * dst->format->BytesPerPixel;

  // 根据像素的字节数进行行填充
  for (int i = 0; i < h; ++i) {
    // 判断格式并做填充
    if (dst->format->BytesPerPixel == 4) {
      // 32位即每个像素4字节
      uint32_t* row_pixel = (uint32_t*) (pixel + i * dst->pitch);
      for (int j = 0; j < w; ++j) {
        row_pixel[j] = color;
      }
    } else if (dst->format->BytesPerPixel == 1) {
      // 8位即每个像素1字节（此部分为假设代码）
      // 实际上此处应该依赖于调色板来选择颜色，但是在本例中我们做简化处理
      memset(pixel + i * dst->pitch, (uint8_t)color, w);
    } else {
      // 如果有其他像素大小/格式类可以添加其他的处理逻辑
      assert(0); // 不支持的像素格式
    }
  }
}

void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h) {
  if (s->format->BitsPerPixel == 32) {
    if (w == 0) w = s->w;
    if (h == 0) h = s->h;

    // 判断是否需要更新整个表面
    if (x == 0 && y == 0 && w == s->w && h == s->h) {
      NDL_DrawRect((uint32_t *)s->pixels, x, y, w, h);
    } else {
      // 更新表面的一部分区域。由于 `NDL_DrawRect` 无法做局部更新
      // 我们需要创建一个新的临时buffer来存储要更新的像素。
      uint32_t *tempPixels = (uint32_t*) malloc(w * h * sizeof(uint32_t));
      assert(tempPixels);

      for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
          // 复制所需像素到临时buffer
          tempPixels[i * w + j] = ((uint32_t*)s->pixels)[(y+i) * s->w + (x+j)];
        }
      }

      // 绘制这部分区域
      NDL_DrawRect(tempPixels, x, y, w, h);

      // 释放临时buffer
      free(tempPixels);
    }
  } else if (s->format->BitsPerPixel == 8) {
    // 8位色彩深度的情况因为色彩需要用查找表来处理
    // 该情况下绘制需要将8位色转换为32位色
    if (w == 0) w = s->w;
    if (h == 0) h = s->h;

    uint32_t *tempPixels = (uint32_t*) malloc(w * h * sizeof(uint32_t));
    assert(tempPixels);

    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        // 获取查找表中的颜色，然后转换为32位色彩
        uint8_t index = ((uint8_t*)s->pixels)[(y+i) * s->w + (x+j)];
        SDL_Color color = s->format->palette->colors[index];
        tempPixels[i * w + j] = translate_color(&color);
      }
    }

    // 绘制这部分区域
    NDL_DrawRect(tempPixels, x, y, w, h);

    // 释放临时buffer
    free(tempPixels);
  }else {
    // 不支持的位深度
    assert(0);
  }
}

// APIs below are already implemented.

static inline int maskToShift(uint32_t mask) {
  switch (mask) {
    case 0x000000ff: return 0;
    case 0x0000ff00: return 8;
    case 0x00ff0000: return 16;
    case 0xff000000: return 24;
    case 0x00000000: return 24; // hack
    default: assert(0);
  }
}

SDL_Surface* SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
    uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  assert(depth == 8 || depth == 32);
  SDL_Surface *s = malloc(sizeof(SDL_Surface));
  assert(s);
  s->flags = flags;
  s->format = malloc(sizeof(SDL_PixelFormat));
  assert(s->format);
  if (depth == 8) {
    s->format->palette = malloc(sizeof(SDL_Palette));
    assert(s->format->palette);
    s->format->palette->colors = malloc(sizeof(SDL_Color) * 256);
    assert(s->format->palette->colors);
    memset(s->format->palette->colors, 0, sizeof(SDL_Color) * 256);
    s->format->palette->ncolors = 256;
  } else {
    s->format->palette = NULL;
    s->format->Rmask = Rmask; s->format->Rshift = maskToShift(Rmask); s->format->Rloss = 0;
    s->format->Gmask = Gmask; s->format->Gshift = maskToShift(Gmask); s->format->Gloss = 0;
    s->format->Bmask = Bmask; s->format->Bshift = maskToShift(Bmask); s->format->Bloss = 0;
    s->format->Amask = Amask; s->format->Ashift = maskToShift(Amask); s->format->Aloss = 0;
  }

  s->format->BitsPerPixel = depth;
  s->format->BytesPerPixel = depth / 8;

  s->w = width;
  s->h = height;
  s->pitch = width * depth / 8;
  assert(s->pitch == width * s->format->BytesPerPixel);

  if (!(flags & SDL_PREALLOC)) {
    s->pixels = malloc(s->pitch * height);
    assert(s->pixels);
  }

  return s;
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
    int pitch, uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) {
  SDL_Surface *s = SDL_CreateRGBSurface(SDL_PREALLOC, width, height, depth,
      Rmask, Gmask, Bmask, Amask);
  assert(pitch == s->pitch);
  s->pixels = pixels;
  return s;
}

void SDL_FreeSurface(SDL_Surface *s) {
  if (s != NULL) {
    if (s->format != NULL) {
      if (s->format->palette != NULL) {
        if (s->format->palette->colors != NULL) free(s->format->palette->colors);
        free(s->format->palette);
      }
      free(s->format);
    }
    if (s->pixels != NULL && !(s->flags & SDL_PREALLOC)) free(s->pixels);
    free(s);
  }
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags) {
  if (flags & SDL_HWSURFACE) NDL_OpenCanvas(&width, &height);
  return SDL_CreateRGBSurface(flags, width, height, bpp,
      DEFAULT_RMASK, DEFAULT_GMASK, DEFAULT_BMASK, DEFAULT_AMASK);
}

void SDL_SoftStretch(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  assert(src && dst);
  assert(dst->format->BitsPerPixel == src->format->BitsPerPixel);
  assert(dst->format->BitsPerPixel == 8);

  int x = (srcrect == NULL ? 0 : srcrect->x);
  int y = (srcrect == NULL ? 0 : srcrect->y);
  int w = (srcrect == NULL ? src->w : srcrect->w);
  int h = (srcrect == NULL ? src->h : srcrect->h);

  assert(dstrect);
  if(w == dstrect->w && h == dstrect->h) {
    /* The source rectangle and the destination rectangle
     * are of the same size. If that is the case, there
     * is no need to stretch, just copy. */
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_BlitSurface(src, &rect, dst, dstrect);
  }
  else {
    assert(0);
  }
}

void SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int firstcolor, int ncolors) {
  assert(s);
  assert(s->format);
  assert(s->format->palette);
  assert(firstcolor == 0);

  s->format->palette->ncolors = ncolors;
  memcpy(s->format->palette->colors, colors, sizeof(SDL_Color) * ncolors);

  if(s->flags & SDL_HWSURFACE) {
    assert(ncolors == 256);
    for (int i = 0; i < ncolors; i ++) {
      uint8_t r = colors[i].r;
      uint8_t g = colors[i].g;
      uint8_t b = colors[i].b;
    }
    SDL_UpdateRect(s, 0, 0, 0, 0);
  }
}

static void ConvertPixelsARGB_ABGR(void *dst, void *src, int len) {
  int i;
  uint8_t (*pdst)[4] = dst;
  uint8_t (*psrc)[4] = src;
  union {
    uint8_t val8[4];
    uint32_t val32;
  } tmp;
  int first = len & ~0xf;
  for (i = 0; i < first; i += 16) {
#define macro(i) \
    tmp.val32 = *((uint32_t *)psrc[i]); \
    *((uint32_t *)pdst[i]) = tmp.val32; \
    pdst[i][0] = tmp.val8[2]; \
    pdst[i][2] = tmp.val8[0];

    macro(i + 0); macro(i + 1); macro(i + 2); macro(i + 3);
    macro(i + 4); macro(i + 5); macro(i + 6); macro(i + 7);
    macro(i + 8); macro(i + 9); macro(i +10); macro(i +11);
    macro(i +12); macro(i +13); macro(i +14); macro(i +15);
  }

  for (; i < len; i ++) {
    macro(i);
  }
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, uint32_t flags) {
  assert(src->format->BitsPerPixel == 32);
  assert(src->w * src->format->BytesPerPixel == src->pitch);
  assert(src->format->BitsPerPixel == fmt->BitsPerPixel);

  SDL_Surface* ret = SDL_CreateRGBSurface(flags, src->w, src->h, fmt->BitsPerPixel,
    fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

  assert(fmt->Gmask == src->format->Gmask);
  assert(fmt->Amask == 0 || src->format->Amask == 0 || (fmt->Amask == src->format->Amask));
  ConvertPixelsARGB_ABGR(ret->pixels, src->pixels, src->w * src->h);

  return ret;
}

uint32_t SDL_MapRGBA(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  assert(fmt->BytesPerPixel == 4);
  uint32_t p = (r << fmt->Rshift) | (g << fmt->Gshift) | (b << fmt->Bshift);
  if (fmt->Amask) p |= (a << fmt->Ashift);
  return p;
}

int SDL_LockSurface(SDL_Surface *s) {
  return 0;
}

void SDL_UnlockSurface(SDL_Surface *s) {
}
