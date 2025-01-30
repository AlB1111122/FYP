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

void* Mmemcpy(void *__restrict dst0,
           const void *__restrict src0,
           size_t len0){
   char *dst = dst0;
     const char *src = src0;
     long *aligned_dst;
     const long *aligned_src;
   
     /* If the size is small, or either SRC or DST is unaligned,
        then punt into the byte copy loop.  This should be rare.  */
     if (!TOO_SMALL(len0) && !UNALIGNED (src, dst))
       {
         aligned_dst = (long*)dst;
         aligned_src = (long*)src;
   
         /* Copy 4X long words at a time if possible.  */
         while (len0 >= BIGBLOCKSIZE)
        {
             *aligned_dst++ = *aligned_src++;
             *aligned_dst++ = *aligned_src++;
             *aligned_dst++ = *aligned_src++;
             *aligned_dst++ = *aligned_src++;
             len0 -= BIGBLOCKSIZE;
           }
   
         /* Copy one long word at a time if possible.  */
         while (len0 >= LITTLEBLOCKSIZE)
           {
             *aligned_dst++ = *aligned_src++;
             len0 -= LITTLEBLOCKSIZE;
           }
   
          /* Pick up any residual with a byte copier.  */
         dst = (char*)aligned_dst;
         src = (char*)aligned_src;
       }
 
    while (len0--)
     *dst++ = *src++;
 
    return dst0;
}

void Mmemmove(void* dest, void* src, unsigned int n)
{
  char* tmp[n];
  Mmemcpy(tmp,src,n);
  Mmemcpy(dest,tmp,n);
}