#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    const char *ptr = s;
    while(*ptr != '\0') {
    	ptr++;
    }
  return ptr - s;
}

char *strcpy(char *dst, const char *src) {
    char *origin_dst = dst;
  while(*src != '\0') {
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0'; // add the null terminator
  return origin_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *origin_dst = dst;
  size_t i;

  // Copy up to n characters from the string pointed to by src to dst
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }

  // If strlen(src) < n, pad the remaining with '\0'
  for ( ; i < n; i++) {
    dst[i] = '\0';
  }

  return origin_dst;
}

char *strcat(char *dst, const char *src) {
  char *dest = dst;
  while (*dst) {
    dst++;
  } 
  while ((*dst++ = *src++));
  return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
    s1++, s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (; n > 0; s1++, s2++, --n){
    if (*s1 != *s2)
      return *(unsigned char *)s1 - *(unsigned char *)s2;
    else if (*s1 == '\0')
      return 0;
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while(n--)
    *p++ = (unsigned char)c;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
      char *cdst = (char *)dst;
    const char *csrc = (const char *)src;

    // If the source and destination overlap such that moving
    // from the beginning to the end would overwrite original data in the source...
    if (csrc < cdst && cdst < csrc + n) {
        // Then we copy from the end to the beginning.
        cdst += n;
        csrc += n;
        while (n--)
            *--cdst = *--csrc;
    } else {
        // Else we can just copy from beginning to end.
        while (n--)
            *cdst++ = *csrc++;
    }
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *dst = out;
  const char *src = in;
  while(n--) {
    *dst++ = *src++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
   const unsigned char *p1 = s1, *p2 = s2;
  while(n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

#endif
