#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static void reverse(char *s, int len) {
  char* end = s + len - 1;
  while (s < end) {
    *s   = *s ^ *end;
    *end = *s ^ *end;
    *s   = *s ^ *end;
    s++;
    end--;
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
int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *start = out;
  
  for (; *fmt != '\0'; ++fmt) {
    if (*fmt != '%') {
      *out = *fmt;
      ++out;
    } else {
      switch (*(++fmt)) {
      case '%': *out = *fmt; ++out; break;
      case 'd': out += itoa(va_arg(ap, int), out, 10); break;
      case 's':
        char *s = va_arg(ap, char*);
        strcpy(out, s);
        out += strlen(out);
        break;
      }
    }
  }
  *out = '\0';

  return out - start;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list pArgs;
  va_start(pArgs, fmt);
  int ret = vsprintf(out, fmt, pArgs);
  va_end(pArgs);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  size_t len = 0;
  

  for (; *fmt != '\0'; ++fmt) {
    if (len >= n) {
      break;
    }

    if (*fmt != '%') {
      *out++ = *fmt;
      len++;
    } else {
      switch (*(++fmt)) {
        case '%':
          if (len < n - 1) {
            *out++ = *fmt;
            len++;
          }
          break;
        case 'd':
          char buffer[32];
          len += itoa(va_arg(ap, int), buffer, 10);
          if (len < n - 1) {
            strcpy(out, buffer);
            out += strlen(out);
          }
          break;
        case 's':
          char *s = va_arg(ap, char*);
          size_t s_len = strlen(s);
          if (len + s_len < n - 1) {
            strcpy(out, s);
            out += s_len;
            len += s_len;
          } else {
            strncpy(out, s, n - len - 1);
            out += n - len - 1;
            len += n - len - 1;
          }
          break;
      }
    }
  }

  if (n > 0) {
    *out = '\0';
  }

  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list pArgs;
  va_start(pArgs, fmt);
  int ret = vsnprintf(out, n, fmt, pArgs);
  va_end(pArgs);
  return ret;
}


#endif
