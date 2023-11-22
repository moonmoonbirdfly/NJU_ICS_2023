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




int vsprintf(char *out, const char *fmt, va_list ap) {
    char *start = out;
    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;

            // Check for zero padding and width specifier
            int zeroPad = 0;
            if (*fmt == '0') {
                fmt++;
                zeroPad = 1;
            }

            int width = 0;
            while(*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt - '0';
                fmt++;
            }

            switch (*fmt) {
            case 'd': { // digit
                int val = va_arg(ap, int);
                int len = itoa(val, out, 10);
                // Zero padding
                if (zeroPad && width > len) {
                    memmove(out + width - len, out, len);
                    memset(out, '0', width - len);
                    out += width;
                } else {
                    out += len;
                }
                break;
            }
            case 's': { // string
                char *s = va_arg(ap, char *);
                strcpy(out, s);
                out += strlen(s);
                break;
            }
            case 'c': { // character
                char val = (char)va_arg(ap, int);  // char promoted to int in va_arg
                *(out++)=val; // increment out after the assignment
                break;
            }
            case '%': { // percent
                *(out++)='%'; // increment out after the assignment
                break;
            }
            default:
                // Unsupported placeholder, copy as-is
                *(out++)='%';
                *(out++)=*fmt;
                break;
            }
        } else {
            *out = *fmt;
            out++;
        }
        fmt++;
    }
    *out = '\0';
    return out - start;
}


int printf(const char *fmt, ...) {
 // create a buffer to store the formatted string.
  // Note: you may want to make this buffer larger or use 
  // dynamic allocation depending on your use case.
 /* char buffer[2048];

  va_list args;
  va_start(args, fmt);

  // Use the vsprintf function to format the string.
  vsprintf(buffer, fmt, args);

  va_end(args);

  // Now print the resulting string to the console. 
  // As this is a demonstrative implementation, it simply uses 
  // a loop and the given `putch` function to display the characters.
  char *ch = buffer;
  while (*ch != '\0') {
    putch(*ch++);
  }

  // Return the number of characters printed
  return ch - buffer;
  
  */
  return 0;
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
