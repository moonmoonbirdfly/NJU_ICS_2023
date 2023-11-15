#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void _putc(char c) {
    // Assume putchar(c) is your system level function to put a char, replace it with your actual function if not.
    putch(c);
}

void print(const char *str) { 
  while (*str != '\0') {
    _putc(*str);
    ++str;
  }
}
void itoa(int n, char s[]) {
    int sign;
    if ((sign = n) < 0)  
        n = -n;

    int i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';

    /* reverse string */
    int start = 0;
    int end = strlen(s) - 1;
    while (start < end) {
        char c = s[start];
        s[start] = s[end];
        s[end] = c;
        start++;
        end--;
    }
}

int printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[1024];
  vsprintf(buf, fmt, args);
  print(buf);
  va_end(args);
  return 0;  // assuming no error
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char* start = out;

  for (char* f = (char *)fmt; *f != '\0'; f++) {
    if (*f != '%') {
      *out = *f;
      out++;
    } else {
      // we need to handle format specifier 
      f++;
      switch(*f) {
        case 'd': {
          int i = va_arg(ap, int);
          char str[12];  // buffer for int
          itoa(i, str);
          strcpy(out, str);
          out += strlen(str);
          break;
        }
        case 's': {
          char* s = va_arg(ap, char*);
          strcpy(out, s);
          out += strlen(s);
          break;
        }
        default: {
          break;
        }
      }
    }
  }

  *out = '\0';  // append null terminator
  return out - start;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int written = vsprintf(out, fmt, args);
  va_end(args);
  return written;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  // This implementation does not respect the 'n' limit, and it is NOT safe.
  // In a complete snprintf implementation, it would write at most 'n' characters.
  va_list args;
  va_start(args, fmt);
  int written = vsprintf(out, fmt, args);
  va_end(args);
  return written;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  // This implementation does not respect the 'n' limit, and it is NOT safe.
  // In a complete vsnprintf implementation, it would write at most 'n' characters.
  int written = vsprintf(out, fmt, ap);
  return written;
}

#endif

