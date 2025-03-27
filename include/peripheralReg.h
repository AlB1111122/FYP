#pragma once
extern "C" {
#include <stdint.h>
}
namespace reg {
constexpr uint32_t PERIPHERAL_BASE = 0xFE000000;
constexpr uint32_t ARM_LOCAL_PERIPHERAL_BASE = 0xFF800000;
constexpr uint32_t MAIN_PERIPHERAL_BASE = 0xFC000000;
constexpr uint32_t GPU_TO_ARM_ADR_MASK = 0x3FFFFFFF;
constexpr uint64_t PERIPHERALS_END = 0x100000000;
}  // namespace reg