#include "../../../include/timer.h"

#include <stdlib.h>

uint64_t Timer::now() {
  uint64_t time = 0x0;
  uint32_t hiCnt = *reinterpret_cast<volatile unsigned int*>(SYS_TIMER_CNT_HI);
  uint32_t loCnt = *reinterpret_cast<volatile unsigned int*>(SYS_TIMER_CNT_LO);
  // account for overeflow as it is a freerunning counter
  if (hiCnt != *reinterpret_cast<volatile unsigned int*>(SYS_TIMER_CNT_HI)) {
    loCnt = 0xFFFEEEEE;  // arbitrary high time close to the end of its flip
  }
  // put the higher counter in the 1st 32 bits of time
  time = static_cast<uint64_t>(
             *reinterpret_cast<volatile unsigned int*>(SYS_TIMER_CNT_HI))
             << 32 |
         loCnt;  // add the contents of the counter low with
                 // bitwise or
  return time;
}

uint64_t Timer::durationSince(uint64_t earlier) {
  uint64_t now = this->now();
  if (didCounterFlip(earlier, now)) {
    return now + earlier;
  }
  return now - earlier;
}

bool Timer::didCounterFlip(uint64_t earlier, uint64_t later) {
  // to be used in cases when it is absolutly known that the order is correct
  // and total time is less than counter flip time
  return later < earlier;
}

int Timer::getHertz() { return this->SYS_TIMER_Hz; }

double Timer::toSec(uint64_t time) {
  lldiv_t divRes = lldiv(time, this->SYS_TIMER_Hz);
  return (double)divRes.quot +
         ((double)divRes.rem / (double)this->SYS_TIMER_Hz);
}

double Timer::toMilli(uint64_t time) {
  // technically can be too big for a double but non issue with where its used
  // for this project limited to 3 decimal places bc timer in microseconds
  lldiv_t divRes = lldiv(time, (this->SYS_TIMER_Hz / 1000));
  return (double)divRes.quot +
         ((double)divRes.rem / (this->SYS_TIMER_Hz / 1000));
}
