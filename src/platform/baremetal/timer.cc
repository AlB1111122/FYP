#include "../../../include/timer.h"
#include <cstdint>

uint64_t Timer::now() {
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


uint64_t Timer::duration_since(uint64_t earlier) {
  uint64_t now = this->now();
  if(did_counter_flip(earlier, now)){
    return now + earlier;
  }else{
    return now - earlier;
  }
}

bool Timer::did_counter_flip(uint64_t earlier,uint64_t later) {
  //to be used in cases when it is absolutly known that the order is correct and total time is less than counter flip time
  if(later < earlier){
    return true;
  }
  return false;
}