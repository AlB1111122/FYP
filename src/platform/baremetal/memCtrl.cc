#include "../../../include/memCtrl.h"

#include <stdint.h>

#include "../../../include/types.h"

void *Mmemcpy(void *__restrict dst0, const void *__restrict src0, size_t len0) {
  char *dst = dst0;
  const char *src = src0;
  long *aligned_dst;
  const long *aligned_src;

  /* If the size is small, or either SRC or DST is unaligned,
     then punt into the byte copy loop.  This should be rare.  */
  if (!TOO_SMALL(len0) && !UNALIGNED(src, dst)) {
    aligned_dst = (long *)dst;
    aligned_src = (long *)src;

    /* Copy 4X long words at a time if possible.  */
    while (len0 >= BIGBLOCKSIZE) {
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      *aligned_dst++ = *aligned_src++;
      len0 -= BIGBLOCKSIZE;
    }

    /* Copy one long word at a time if possible.  */
    while (len0 >= LITTLEBLOCKSIZE) {
      *aligned_dst++ = *aligned_src++;
      len0 -= LITTLEBLOCKSIZE;
    }

    /* Pick up any residual with a byte copier.  */
    dst = (char *)aligned_dst;
    src = (char *)aligned_src;
  }

  while (len0--) *dst++ = *src++;

  return dst0;
}

void Mmemmove(void *dst_void, const void *src_void, size_t length) {
  char *dst = dst_void;
  const char *src = src_void;
  long *aligned_dst;
  const long *aligned_src;

  if (src < dst && dst < src + length) {
    /* Destructive overlap...have to copy backwards */
    src += length;
    dst += length;
    while (length--) {
      *--dst = *--src;
    }
  } else {
    /* Use optimizing algorithm for a non-destructive copy to closely
       match memcpy. If the size is small or either SRC or DST is unaligned,
       then punt into the byte copy loop.  This should be rare.  */
    if (!TOO_SMALL(length) && !UNALIGNED(src, dst)) {
      aligned_dst = (long *)dst;
      aligned_src = (long *)src;

      /* Copy 4X long words at a time if possible.  */
      while (length >= BIGBLOCKSIZE) {
        *aligned_dst++ = *aligned_src++;
        *aligned_dst++ = *aligned_src++;
        *aligned_dst++ = *aligned_src++;
        *aligned_dst++ = *aligned_src++;
        length -= BIGBLOCKSIZE;
      }

      /* Copy one long word at a time if possible.  */
      while (length >= LITTLEBLOCKSIZE) {
        *aligned_dst++ = *aligned_src++;
        length -= LITTLEBLOCKSIZE;
      }

      /* Pick up any residual with a byte copier.  */
      dst = (char *)aligned_dst;
      src = (char *)aligned_src;
    }

    while (length--) {
      *dst++ = *src++;
    }
  }

  return dst_void;
}

void clean_invalidate_cache(void *buffer, unsigned long size) {
  uintptr_t addr = (uintptr_t)buffer;
  uintptr_t end = addr + size;

  // from docs
  size_t cache_line_size = 64;

  __asm__ volatile(
      "1:\n"
      "dc ivac, %0\n"     // invalidate cache line
      "add %0, %0, %1\n"  // next cache line
      "cmp %0, %2\n"      // chaeck if done
      "b.lt 1b\n"
      "dsb sy\n"
      "isb\n"
      : "+r"(addr)  // params of assembly function
      : "r"(cache_line_size), "r"(end)
      : "memory");
}
