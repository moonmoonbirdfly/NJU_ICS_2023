#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}



// Remaining functions... 

void *start_of_heap = NULL; // Store the start of heap.

void *malloc(size_t size) {
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  if (start_of_heap == NULL) {
    start_of_heap = (void *)heap.start; // Initialize if not yet initiated.
  }
  
  if ((start_of_heap + size) > (void*)heap.end) {
    panic("Heap Overflow!");
    return NULL; // Not enough space left on the heap.
  }

  void *old_heap = start_of_heap;

  start_of_heap += size;
  
  return old_heap;
#else
  panic("Not implemented");
  return NULL;
#endif
}

void free(void *ptr) {
  // Do nothing. Recall that our malloc also doesn't really free any memory.
}

#endif
