#pragma once
extern "C" {
#include <stdint.h>
}
namespace reg {
constexpr uint32_t PERIPHERAL_BASE = 0xFE000000;
constexpr uint32_t ARM_LOCAL_PERIPHERAL_BASE = 0xFF800000;
constexpr uint32_t MAIN_PERIPHERAL_BASE = 0xFC000000;
}  // namespace reg