// void mailboxWrite(unsigned char channel)
// {
//     // Gpio gpio = Gpio();
//     // // 28-bit address (MSB) and 4-bit value (LSB) pass the adress to the actuall mailbox hw
//     unsigned int mbox_request_addr = ((unsigned int)((long) &mbox) &~ 0xF) | (channel & 0xF);

//     // volatile unsigned int mbox_request_addr = &mbox;
//     // mbox_request_addr &= ~(0xF); // clear last 4 LSB
//     // mbox_request_addr |= channel; //put the channel in the last 4 bits 

//     // spin until we can write
//     //while (gpio.mmio_read(MBOX_STATUS) & MBOX_FULL);
//     while( ( *(volatile unsigned int *)MBOX_STATUS & MBOX_FULL ) != 0 ) {
//         ;
//     }

//     *(volatile unsigned int *)MBOX_WRITE = mbox_request_addr;
    
//     // Write the address of our buffer to the mailbox with the channel appended
//     // gpio.mmio_write(MBOX_WRITE, r);

//     // while (1) {
//     //     // Is there a reply?
//     //     while (gpio.mmio_read(MBOX_STATUS) & MBOX_EMPTY);

//     //     // Is it a reply to our message?
//     //     if (r == gpio.mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
           
//     // }
//     // return 0;

//     // value &= ~(0xF);
//     // value |= channel;

//     // /* Wait until the mailbox becomes available and then write to the mailbox
//     //    channel */
//     // while( ( rpiMailbox0->Status & ARM_MS_FULL ) != 0 ) { }

//     // /* Write the modified value + channel number into the write register */
//     // rpiMailbox0->Write = value;
// }

// int mailboxRead(unsigned char channel)
// {
//     /* For information about accessing mailboxes, see:
//        https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes */
//     int value = -1;

//     /* Keep reading the register until the desired channel gives us a value */
//     while ( ( value & 0xF ) != channel ) {
//         /* Wait while the mailbox is empty because otherwise there's no value
//            to read! */
//         while( ( *(volatile unsigned int *)MBOX_STATUS & MBOX_EMPTY ) != 0 ) {
//             ;
//         }
//         value = *(volatile unsigned int *)MBOX_READ ;

//         /* Extract the value from the Read register of the mailbox. The value
//            is actually in the upper 28 bits */
//         //value = rpiMailbox0->Read;
//         // if (value == *(volatile unsigned int *)MBOX_READ) {
//         //     return mbox[1]==MBOX_RESPONSE;
//         // };
//     }
//     return value >> 4;

//     /* Return just the value (the upper 28-bits) */
//     //return value >> 4;
// }

//     // while (1) {
//     //     // Is there a reply?
//     //     while (mmio_read(MBOX_STATUS) & MBOX_EMPTY);

//     //     // Is it a reply to our message?
//     //     if (r == mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
           
//     // }
//     // return 0;
#include "../../../include/ARMMailbox.h"

unsigned int ARMMailbox::mbox_call(unsigned char channel)
{
    Gpio gpio = Gpio();
    // 28-bit address (MSB) and 4-bit value (LSB)
    unsigned int r = ((unsigned int)((long) &mbox) &~ 0xF) | (channel & 0xF);

    // Wait until we can write
    while (gpio.mmio_read(MBOX_STATUS) & MBOX_FULL);
    
    // Write the address of our buffer to the mailbox with the channel appended
    gpio.mmio_write(MBOX_WRITE, r);

    while (1) {
        // Is there a reply?
        while (gpio.mmio_read(MBOX_STATUS) & MBOX_EMPTY);

        // Is it a reply to our message?
        if (r == gpio.mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
           
    }
    return 0;
}

// unsigned int ARMMailbox::mailboxRead(unsigned char channel)
// {
//     Gpio gpio = Gpio();
//     // 28-bit address (MSB) and 4-bit value (LSB)
//     while (gpio.mmio_read(MBOX_STATUS) & MBOX_EMPTY){};

//     // Is it a reply to our message?
//     if (r == gpio.mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
// }
