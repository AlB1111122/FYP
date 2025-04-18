
#pragma once

// cant be in class bc it needs to be used in assembly
extern "C" void init_mmu();

#ifndef __ASSEMBLER__

// defined in boot.s
extern "C" void memzero(unsigned long src, unsigned int n);
#endif