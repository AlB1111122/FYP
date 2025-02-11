#include "../../../include/ARMMailbox.h"

unsigned int ARMMailbox::mailboxWriteRead(unsigned char channel)
{
    // 28-bit address (MSB) and 4-bit value (LSB)
    unsigned int message = ((unsigned int)((long) &mbox) &~ 0xF) | (channel & 0xF);

    // Wait until we can write
    while (gpio.mmio_read(MBOX_STATUS) & MBOX_FULL);
    
    // Write the address of our buffer to the mailbox with the channel appended
    gpio.mmio_write(MBOX_WRITE, message);

    while (1) {
        // Is there a reply?
        while (gpio.mmio_read(MBOX_STATUS) & MBOX_EMPTY){};

        // Is it a reply to our message?
        if (message == gpio.mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
           
    }
    return 0;
}

void ARMMailbox::mailboxWrite(unsigned char channel)
{//set the values of mailbox before this func
    // 28-bit address (MSB) and 4-bit value (LSB)
    unsigned int message = ((unsigned int)((long) &mbox) &~ 0xF) | (channel & 0xF);
    while (gpio.mmio_read(MBOX_STATUS) & MBOX_FULL){;};

    gpio.mmio_write(MBOX_WRITE, message);
}

unsigned int ARMMailbox::mailboxRead(unsigned char channel){
    while (1) {
        // Is there a reply?
        while (gpio.mmio_read(MBOX_STATUS) & MBOX_EMPTY){;};
        unsigned int data = gpio.mmio_read(MBOX_READ);
        unsigned char readChannel = data & 0xF;
        data >>= 4;
		// Return it straight away if it's for the requested channel
		if (readChannel == channel)
			return data;
    }
}
