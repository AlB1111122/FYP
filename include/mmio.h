#pragma once
#include <stdint.h>

class MMIO {
  // mmio writes and reads
 public:
  void write(uintptr_t reg, uint32_t value);

  uint32_t read(uintptr_t reg) const;
};