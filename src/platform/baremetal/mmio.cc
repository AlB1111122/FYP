#include "mmio.h"

void MMIO::write(uintptr_t reg, uint32_t value) {
  // dont overwite anything
  __asm__ volatile("dmb sy" ::: "memory");
  *reinterpret_cast<volatile uint32_t *>(reg) = value;
  // ensure completion
  __asm__ volatile("dmb sy" ::: "memory");
}

uint32_t MMIO::read(uintptr_t reg) const {
  // dont read something irrelevent
  __asm__ volatile("dmb sy" ::: "memory");
  uint32_t val = *reinterpret_cast<volatile uint32_t *>(reg);
  return val;
}