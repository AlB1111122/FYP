#include <stdint.h>
#include "../../../include/types.h"
#include "../../../include/memCtrl.h"

void SMmemcpy(void *dst, void *src, unsigned int len)
{
    long * plDst = (long *) dst;
    long const * plSrc = (long const *) src;

    if (!((uintptr_t)src & 0xFFFFFFFC) && !((uintptr_t)dst & 0xFFFFFFFC))
    {
        while (len >= 4)
    {
            *plDst++ = *plSrc++;
            len -= 4;
        }
    }

    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;

    while (len--)
    {
        *pcDst++ = *pcSrc++;
    }

    return (dst);
 }

void SMmemmove(void* dest, void* src, unsigned int n)
{
    char *pDest = (char *)dest;
    const char *pSrc =( const char*)src;
    //allocate memory for tmp array
    char *tmp[n];
        unsigned int i = 0;
        // copy src to tmp array
        for(i =0; i < n ; ++i)
        {
            *(tmp + i) = *(pSrc + i);
        }
        //copy tmp to dest
        for(i =0 ; i < n ; ++i)
        {
            *(pDest + i) = *(tmp + i);
        }
}

void Mmemcpy(void *dest, void *src, unsigned int n){
    char *d = (char *)dest;
  const char *s = (char *)src;

  if (n < 5) {
    if (n == 0)
      return dest;
    d[0] = s[0];
    d[n - 1] = s[n - 1];
    if (n <= 2)
      return dest;
    d[1] = s[1];
    d[2] = s[2];
    return dest;
  }

  if (n <= 16) {
    if (n >= 8) {
      const char *first_s = s;
      const char *last_s = s + n - 8;
      char *first_d = d;
      char *last_d = d + n - 8;
      *((u64 *)first_d) = *((u64 *)first_s);
      *((u64 *)last_d) = *((u64 *)last_s);
      return dest;
    }

    const char *first_s = s;
    const char *last_s = s + n - 4;
    char *first_d = d;
    char *last_d = d + n - 4;
    *((u32 *)first_d) = *((u32 *)first_s);
    *((u32 *)last_d) = *((u32 *)last_s);
    return dest;
  }

  if (n <= 32) {
    const char *first_s = s;
    const char *last_s = s + n - 16;
    char *first_d = d;
    char *last_d = d + n - 16;

    *((char16 *)first_d) = *((char16 *)first_s);
    *((char16 *)last_d) = *((char16 *)last_s);
    return dest;
  }

  const char *last_word_s = s + n - 32;
  char *last_word_d = d + n - 32;

  // Stamp the 32-byte chunks.
  do {
    *((char32 *)d) = *((char32 *)s);
    d += 32;
    s += 32;
  } while (d < last_word_d);

  // Stamp the last unaligned 32 bytes of the buffer.
  *((char32 *)last_word_d) = *((char32 *)last_word_s);
  return dest;
}

void Mmemmove(void* dest, void* src, unsigned int n)
{
  char* tmp[n];
  Mmemcpy(tmp,src,n);
  Mmemcpy(dest,tmp,n);
}