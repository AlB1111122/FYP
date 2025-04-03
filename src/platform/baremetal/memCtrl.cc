#include "../../../include/memCtrl.h"

#include <stdint.h>

void cleanInvalidateCache(void *buffer, unsigned long size) {
  uintptr_t addr = (uintptr_t)buffer;
  uintptr_t end = addr + size;

  // from docs
  size_t cache_line_size = 64;

  __asm__ volatile(
      "1:\n"
      "dc ivac, %0\n"     // invalidate cache line
      "add %0, %0, %1\n"  // next cache line
      "cmp %0, %2\n"      // check if done
      "b.lt 1b\n"
      "dsb sy\n"
      "isb\n"
      : "+r"(addr)  // params of assembly function
      : "r"(cache_line_size), "r"(end)
      : "memory");
}
