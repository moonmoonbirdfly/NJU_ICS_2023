#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static void reverse(char *s, int len) {
  char *end = s + len - 1;
  char tmp;
  while (s < end) {
    tmp = *s;
    *s = *end;
    *end = tmp;
  }
}

/* itoa convert int to string under base. return string length */
static int itoa(int n, char *s, int base) {
  assert(base <= 16);

  int i = 0, sign = n, bit;
  if (sign < 0) n = -n;
  do {
    bit = n % base;
    if (bit >= 10) s[i++] = 'a' + bit - 10;
    else s[i++] = '0' + bit;
  } while ((n /= base) > 0);
  if (sign < 0) s[i++] = '-';
  s[i] = '\0';
  reverse(s, i);

  return i;
}
void outchar(char *out, int *index, char c) {
  out[*index] = c;
  ++(*index);
}

void outstring(char *out, int *index, char *s) {
  while(*s != '\0') {
    outchar(out, index, *s++);
  }
}

void outint(char *out, int *index, int i) {
  char buffer[32];
  itoa(i, buffer, 10);
  outstring(out, index, buffer);
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  const char *p;
  int out_index = 0;

  for (p = fmt; *p != '\0'; p++) {
    if(*p != '%') {
      outchar(out, &out_index, *p);
      continue;
    }
    ++p;
    switch(*p) {
      case 'd':
        outint(out, &out_index, va_arg(ap, int));
        break;
      case 's':
        outstring(out, &out_index, va_arg(ap, char*));
        break;
      default:
        outchar(out, &out_index, *p);
        break;
    }
  }
  out[out_index] = '\0';
  return out_index;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  int ret = vsprintf(out, fmt, argp);
  va_end(argp);
  return ret;
}

#endif

