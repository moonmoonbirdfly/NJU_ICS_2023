#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
//static void reverse(char *s, int len) {
//  char* end = s + len - 1;
//  while (s < end) {
//    *s   = *s ^ *end;
//    *end = *s ^ *end;
//    *s   = *s ^ *end;
//    s++;
//    end--;
//  }
//}


/* itoa convert int to string under base. return string length */
static char numToChar(int num) {
    if (num < 10)
        return num + '0';
    else
        return num - 10 + 'a';
}

static int itoa(int n, char *s, int base) {
  assert(base <= 16);
  
  char buf[32];
  int bit;
  int i = 0;
  int start = 0;
  
  if (n == 0) {
    buf[i++] = '0';
  } else {
    while (n != 0) {
      bit = n % base;
      buf[i++] = numToChar(bit);
      n /= base;
    }
  }

  if (base == 10 && n < 0)
    buf[i++] = '-';

  while (i > 0) {
    s[start++] = buf[--i];
  }
  s[start] = '\0';
  
  return start;
}



static char *copy_string(char *out, const char *str) {
    char *start = out;

    while (*str) {
        *out++ = *str++;
    }

    return start;
}

static char *convert_to_dec_string(char *out, int num) {
    char buffer[32];
    itoa(num, buffer, 10);
    return copy_string(out, buffer);
}

static char sprint_buf[1024];
int printf(const char *fmt, ...) {


    char *ptr = sprint_buf;
    while (*ptr != '\0') {
        putch(*ptr++);
    }
    return ptr - sprint_buf; // return the number of characters printed
}




int vsprintf(char *out, const char *fmt, va_list ap) {
    char *start = out;

    for (; *fmt != '\0'; ++fmt) {
        if (*fmt != '%') {
            *out++ = *fmt;
        } else {
            switch (*(++fmt)) {
                case '%':
                    *out++ = *fmt;
                    break;
                case 'd':
                    out = convert_to_dec_string(out, va_arg(ap, int));
                    break;
                case 's':
                    out = copy_string(out, va_arg(ap, char*));
                    break;
                case 'c':
                    *out++ = (char)va_arg(ap, int);
                    break;
            }
        }
    }
    
    *out = '\0';
    
    return out - start;
}

//static char sprint_buf[1024];


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
