#pragma once
#include <stddef.h>
#include <stdint.h>
// defined in boot.s
extern "C" void memzero(unsigned long src, unsigned int n);
void cleanInvalidateCache(void *buffer, unsigned long size);
