#include "../../../include/gpio.h"
#include "../../../include/hdmiOut.h"
#include "../../../include/terminal.h"
#include "../../../include/memCtrl.h"
#include "../../../include/ARMMailbox.h"

unsigned int width, height, pitch, isrgb; //7680 pitch
unsigned char *fb;
ARMMailbox FB_mailbox;

void fb_init()
{
    FB_mailbox.mbox[0] = 35*4; // Length of message in bytes
    FB_mailbox.mbox[1] = MBOX_REQUEST;

    FB_mailbox.mbox[2] = MBOX_TAG_SETPHYWH; // Tag identifier
    FB_mailbox.mbox[3] = 8; // Value size in bytes
    FB_mailbox.mbox[4] = 0;
    FB_mailbox.mbox[5] = 1920; // Value(width)
    FB_mailbox.mbox[6] = 1080; // Value(height)

    FB_mailbox.mbox[7] = MBOX_TAG_SETVIRTWH;
    FB_mailbox.mbox[8] = 8;
    FB_mailbox.mbox[9] = 8;
    FB_mailbox.mbox[10] = 1920;
    FB_mailbox.mbox[11] = 1080;

    FB_mailbox.mbox[12] = MBOX_TAG_SETVIRTOFF;
    FB_mailbox.mbox[13] = 8;
    FB_mailbox.mbox[14] = 8;
    FB_mailbox.mbox[15] = 0; // Value(x)
    FB_mailbox.mbox[16] = 0; // Value(y)

    FB_mailbox.mbox[17] = MBOX_TAG_SETDEPTH;
    FB_mailbox.mbox[18] = 4;
    FB_mailbox.mbox[19] = 4;
    FB_mailbox.mbox[20] = 32; // Bits per pixel

    FB_mailbox.mbox[21] = MBOX_TAG_SETPXLORDR;
    FB_mailbox.mbox[22] = 4;
    FB_mailbox.mbox[23] = 4;
    FB_mailbox.mbox[24] = 1; // RGB

    FB_mailbox.mbox[25] = MBOX_TAG_GETFB;
    FB_mailbox.mbox[26] = 8;
    FB_mailbox.mbox[27] = 8;
    FB_mailbox.mbox[28] = 4096; // FrameBufferInfo.pointer
    FB_mailbox.mbox[29] = 0;    // FrameBufferInfo.size

    FB_mailbox.mbox[30] = MBOX_TAG_GETPITCH;
    FB_mailbox.mbox[31] = 4;
    FB_mailbox.mbox[32] = 4;
    FB_mailbox.mbox[33] = 0; // Bytes per line

    FB_mailbox.mbox[34] = MBOX_TAG_LAST;

    // Check call is successful and we have a pointer with depth 32
    if (FB_mailbox.mbox_call(MBOX_CH_PROP) && FB_mailbox.mbox[20] == 32 && FB_mailbox.mbox[28] != 0) {
        FB_mailbox.mbox[28] &= 0x3FFFFFFF; // Convert GPU address to ARM address
        width = FB_mailbox.mbox[10];       // Actual physical width
        height = FB_mailbox.mbox[11];      // Actual physical height
        pitch = FB_mailbox.mbox[33];       // Number of bytes per line
        isrgb = FB_mailbox.mbox[24];       // Pixel order
        fb = (unsigned char *)((long)FB_mailbox.mbox[28]);
    }
}

unsigned char * getFb(){
    return fb;
}

void drawPixel(int x, int y, unsigned char attr)//comp this to putpixel and see how we go
{
    int offs = (y * pitch) + (x * 4);
    *((unsigned int*)(fb + offs)) = vgapal[attr & 0x0f];
}

void drawPixelRGB(int x, int y, unsigned int colourRGB)
{
    int offs = (y * pitch) + (x * 4);
    *((unsigned int*)(fb + offs)) = colourRGB;
}

void drawByLine(uint8_t* buffer,int xSz =1280*4,int ySz =720)
{
    for(int i =0;i<ySz;i++){
        unsigned int offs = i*pitch;
        Mmemcpy((fb + offs),(buffer + (i*xSz)),xSz);
    }
}

void bufferCpy(uint8_t* buffer,long n)
{
    Mmemcpy(fb,buffer,n);
}

void drawChar(unsigned char ch, int x, int y, unsigned char attr)
{
    unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

    for (int i=0;i<FONT_HEIGHT;i++) {
	for (int j=0;j<FONT_WIDTH;j++) {
	    unsigned char mask = 1 << j;
	    unsigned char col = (*glyph & mask) ? attr & 0x0f : (attr & 0xf0) >> 4;

	    drawPixel(x+j, y+i, col);
	}
	glyph += FONT_BPL;
    }
}

void drawString(int x, int y, char *s, unsigned char attr)
{
    while (*s) {
       if (*s == '\r') {
          x = 0;
       } else if(*s == '\n') {
          x = 0; y += FONT_HEIGHT;
       } else {
	  drawChar(*s, x, y, attr);
          x += FONT_WIDTH;
       }
       s++;
    }
}

// // The buffer must be 16-byte aligned as only the upper 28 bits of the address can be passed via the mailbox
// volatile unsigned int __attribute__((aligned(16))) mbox[36];

// enum {
//     VIDEOCORE_MBOX = (reg::PERIPHERAL_BASE + 0x0000B880),
//     MBOX_READ      = (VIDEOCORE_MBOX + 0x0),
//     MBOX_POLL      = (VIDEOCORE_MBOX + 0x10),
//     MBOX_SENDER    = (VIDEOCORE_MBOX + 0x14),
//     MBOX_STATUS    = (VIDEOCORE_MBOX + 0x18),
//     MBOX_CONFIG    = (VIDEOCORE_MBOX + 0x1C),
//     MBOX_WRITE     = (VIDEOCORE_MBOX + 0x20),
//     MBOX_RESPONSE  = 0x80000000,
//     MBOX_FULL      = 0x80000000,
//     MBOX_EMPTY     = 0x40000000
// };

// unsigned int mbox_call(unsigned char ch)
// {
//     Gpio gpio = Gpio();
//     // 28-bit address (MSB) and 4-bit value (LSB)
//     unsigned int r = ((unsigned int)((long) &mbox) &~ 0xF) | (ch & 0xF);

//     // Wait until we can write
//     while (gpio.mmio_read(MBOX_STATUS) & MBOX_FULL);
    
//     // Write the address of our buffer to the mailbox with the channel appended
//     gpio.mmio_write(MBOX_WRITE, r);

//     while (1) {
//         // Is there a reply?
//         while (gpio.mmio_read(MBOX_STATUS) & MBOX_EMPTY);

//         // Is it a reply to our message?
//         if (r == gpio.mmio_read(MBOX_READ)) return mbox[1]==MBOX_RESPONSE; // Is it successful?
           
//     }
//     return 0;
// }
