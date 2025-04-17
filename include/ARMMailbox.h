#pragma once
#include "gpio.h"
#include "mmio.h"
#include "peripheralReg.h"

class ARMMailbox {
 public:
  unsigned int writeRead(unsigned char channel);
  unsigned int read(unsigned char channel);
  void write(unsigned char channel);
  // The buffer must be 16-byte aligned as only the upper 28 bits of the address
  // can be passed via the mailbox
  volatile unsigned int __attribute__((aligned(16))) mbox[36];

 private:
  MMIO mmio;
  enum {
    VIDEOCORE_MBOX = (reg::PERIPHERAL_BASE + 0x0000B880),
    MBOX_READ = (VIDEOCORE_MBOX + 0x0),
    MBOX_POLL = (VIDEOCORE_MBOX + 0x10),
    MBOX_SENDER = (VIDEOCORE_MBOX + 0x14),
    MBOX_STATUS = (VIDEOCORE_MBOX + 0x18),
    MBOX_CONFIG = (VIDEOCORE_MBOX + 0x1C),
    MBOX_WRITE = (VIDEOCORE_MBOX + 0x20),
    MBOX_RESPONSE = 0x80000000,
    MBOX_FULL = 0x80000000,
    MBOX_EMPTY = 0x40000000
  };
};
