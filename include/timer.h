#pragma once
extern "C" {
#include <stdint.h>
}
#include "peripheral_reg.h"

class Timer {
 private:
  enum {
    SYS_TIMER_BASE = reg::MAIN_PERIPHERAL_BASE + 0x2003000,
    SYS_TIMER_CTRL = SYS_TIMER_BASE + 0x0,
    SYS_TIMER_CNT_LO = SYS_TIMER_BASE + 0x04,
    SYS_TIMER_CNT_HI = SYS_TIMER_BASE + 0x08,
    SYS_TIMER_CMP_0 = SYS_TIMER_BASE + 0x0c,
    SYS_TIMER_CMP_1 = SYS_TIMER_BASE + 0x10,
    SYS_TIMER_CMP_2 = SYS_TIMER_BASE + 0x14,
    SYS_TIMER_CMP_3 = SYS_TIMER_BASE + 0x18
  };

 public:
  static uint64_t readTimer();
};