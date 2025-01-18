#ifndef MEMCPY_AARCH64_H
#define MEMCPY_AARCH64_H

#include <stddef.h>

// defined in assembly  boot.s
extern "C" void *__memcpy_aarch64(void *dest, const void *src, size_t n);

#endif // MEMCPY_AARCH64_H