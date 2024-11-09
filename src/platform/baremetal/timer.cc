#include "../../../include/timer.h"

uint64_t Timer::readTimer() {
  uint64_t time = 0x0;
  uint32_t hi_cnt = *(volatile unsigned int *)SYS_TIMER_CNT_HI;
  uint32_t lo_cnt = *(volatile unsigned int *)SYS_TIMER_CNT_LO;
  // account for overeflow as it is a freerunning counter
  if (hi_cnt != *(volatile unsigned int *)SYS_TIMER_CNT_HI) {
    lo_cnt = 0xFFFEEEEE;  // arbitrary high time close to the end of its flip
  }
  // put the higher counter in the 1st 32 bits of time
  time = static_cast<uint64_t>(*(volatile unsigned int *)SYS_TIMER_CNT_HI)
             << 32 |
         lo_cnt;  // add the contents of the counter low with
                  // bitwize or
  return time;
}