#pragma once
extern "C" {
#include <stdint.h>
}
#include "peripheralReg.h"

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
  int SYS_TIMER_Hz = 1000000;
  bool didCounterFlip(uint64_t earlier, uint64_t later) const;

 public:
  static uint64_t now();
  uint64_t durationSince(uint64_t earlier) const;
  int getHertz() const;
  double toSec(uint64_t time) const;
  double toMilli(uint64_t time) const;
};