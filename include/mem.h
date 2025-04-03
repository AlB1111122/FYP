
#pragma once
#include "memCtrl.h"

void *get_free_pages(int num_pages);
void *allocate_memory(int bytes);
void free_memory(void *base);
// cant be in class bc it needs to be used in assembly
extern "C" void init_mmu();

#ifndef __ASSEMBLER__

extern "C" void memzero(unsigned long src, unsigned int n);
#endif