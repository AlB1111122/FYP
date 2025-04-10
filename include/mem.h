
#pragma once
#include "memCtrl.h"

// cant be in class bc it needs to be used in assembly
extern "C" void init_mmu();

#ifndef __ASSEMBLER__

extern "C" void memzero(unsigned long src, unsigned int n);
#endif