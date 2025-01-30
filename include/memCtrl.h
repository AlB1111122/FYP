//extern "C" void Mmemcpy(void *dest, void *src, unsigned int n);
//extern "C" void Mmemmove(void* dest, void* src, unsigned int n);

#include <stddef.h>
#include <stdint.h>

   #define UNALIGNED(X, Y) \
     (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))
   
   /* How many bytes are copied each iteration of the 4X unrolled loop.  */
   #define BIGBLOCKSIZE    (sizeof (long) << 2)
   
   /* How many bytes are copied each iteration of the word copy loop.  */
   #define LITTLEBLOCKSIZE (sizeof (long))
   
   /* Threshhold for punting to the byte copier.  */
   #define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)
void Mmemmove(void *dest, void *src, unsigned int n);
void* Mmemcpy(void *__restrict dst0,
           const void *__restrict src0,
           size_t len0);
void SMmemcpy(void *dest, void *src,  unsigned int n);
void SMmemmove(void* dest, void* src, unsigned int n);
