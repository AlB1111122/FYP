#include "../../../include/ARMMailbox.h"

unsigned int ARMMailbox::writeRead(
    unsigned char channel) {  // set the values of mailbox before this func
  uintptr_t addr = reinterpret_cast<uintptr_t>(&mbox);
  // 28-bit address (MSB) and 4-bit value (LSB)
  unsigned int message =
      static_cast<unsigned int>(addr & ~0xF) | (channel & 0xF);

  // Wait until we can write
  while (gpio.mmioRead(MBOX_STATUS) & MBOX_FULL)
    ;

  // Write the address of our buffer to the mailbox with the channel appended
  gpio.mmioWrite(MBOX_WRITE, message);

  while (true) {
    // Is there a reply?
    while (gpio.mmioRead(MBOX_STATUS) & MBOX_EMPTY) {
      ;
    }

    // Is it a reply to our message?
    if (message == gpio.mmioRead(MBOX_READ)) {
      return mbox[1] == MBOX_RESPONSE;  // Is it successful?
    }
  }
  return 0;
}

void ARMMailbox::write(
    unsigned char channel) {  // set the values of mailbox before this func
  uintptr_t addr = reinterpret_cast<uintptr_t>(&mbox);
  // 28-bit address (MSB) and 4-bit value (LSB)
  unsigned int message =
      static_cast<unsigned int>(addr & ~0xF) | (channel & 0xF);
  while (gpio.mmioRead(MBOX_STATUS) & MBOX_FULL) {
    ;
  }

  gpio.mmioWrite(MBOX_WRITE, message);
}

unsigned int ARMMailbox::read(unsigned char channel) {
  while (true) {
    // check reply
    while (gpio.mmioRead(MBOX_STATUS) & MBOX_EMPTY) {
      ;
    }
    unsigned int data = gpio.mmioRead(MBOX_READ);
    unsigned char readChannel = data & 0xF;
    data >>= 4;
    // Return it straight away if it's for the requested channel
    if (readChannel == channel) {
      return data;
    }
  }
}
