// #include "../../../include/gpio.h"
// #include "../../../include/frameBuffer.h"
// #include "../../../include/terminal.h"
// #include "../../../include/memCtrl.h"

// FrameBuffer::FrameBuffer()
// {
//     FB_mailbox.mbox[0] = 35*4; // Length of message in bytes
//     FB_mailbox.mbox[1] = MBOX_REQUEST;

//     FB_mailbox.mbox[2] = MBOX_TAG_SETPHYWH; // Tag identifier
//     FB_mailbox.mbox[3] = 8; // Value size in bytes
//     FB_mailbox.mbox[4] = 0;
//     FB_mailbox.mbox[5] = 1920; // Value(width)
//     FB_mailbox.mbox[6] = 1080; // Value(height)

//     FB_mailbox.mbox[7] = MBOX_TAG_SETVIRTWH;
//     FB_mailbox.mbox[8] = 8;
//     FB_mailbox.mbox[9] = 8;
//     FB_mailbox.mbox[10] = 1920;
//     FB_mailbox.mbox[11] = 1080;

//     FB_mailbox.mbox[12] = MBOX_TAG_SETVIRTOFF;
//     FB_mailbox.mbox[13] = 8;
//     FB_mailbox.mbox[14] = 8;
//     FB_mailbox.mbox[15] = 0; // Value(x)
//     FB_mailbox.mbox[16] = 0; // Value(y)

//     FB_mailbox.mbox[17] = MBOX_TAG_SETDEPTH;
//     FB_mailbox.mbox[18] = 4;
//     FB_mailbox.mbox[19] = 4;
//     FB_mailbox.mbox[20] = 32; // Bits per pixel

//     FB_mailbox.mbox[21] = MBOX_TAG_SETPXLORDR;
//     FB_mailbox.mbox[22] = 4;
//     FB_mailbox.mbox[23] = 4;
//     FB_mailbox.mbox[24] = 1; // RGB

//     FB_mailbox.mbox[25] = MBOX_TAG_GETFB;
//     FB_mailbox.mbox[26] = 8;
//     FB_mailbox.mbox[27] = 8;
//     FB_mailbox.mbox[28] = 4096; // FrameBufferInfo.pointer
//     FB_mailbox.mbox[29] = 0;    // FrameBufferInfo.size

//     FB_mailbox.mbox[30] = MBOX_TAG_GETPITCH;
//     FB_mailbox.mbox[31] = 4;
//     FB_mailbox.mbox[32] = 4;
//     FB_mailbox.mbox[33] = 0; // Bytes per line

//     FB_mailbox.mbox[34] = MBOX_TAG_LAST;

//     // Check call is successful and we have a pointer with depth 32
//     if (FB_mailbox.mbox_call(MBOX_CH_PROP) && FB_mailbox.mbox[20] == 32 && FB_mailbox.mbox[28] != 0) {
//         FB_mailbox.mbox[28] &= 0x3FFFFFFF; // Convert GPU address to ARM address
//         this->width = FB_mailbox.mbox[10];       // Actual physical width
//         this->height = FB_mailbox.mbox[11];      // Actual physical height
//         this->pitch = FB_mailbox.mbox[33];       // Number of bytes per line
//         this->isrgb = FB_mailbox.mbox[24];       // Pixel order
//         this->fb = (unsigned char *)((long)FB_mailbox.mbox[28]);
//     }
// }

// unsigned char * FrameBuffer::getFb(){
//     return this->fb;
// }

// void FrameBuffer::drawPixel(int x, int y, unsigned char attr)//comp this to putpixel and see how we go
// {
//     int offs = (y * this->pitch) + (x * 4);
//     *((unsigned int*)(this->fb + offs)) = vgapal[attr & 0x0f];
// }

// void FrameBuffer::drawPixelRGB(int x, int y, unsigned int colourRGB)
// {
//     int offs = (y * this->pitch) + (x * 4);
//     *((unsigned int*)(this->fb + offs)) = colourRGB;
// }

// void FrameBuffer::drawByLine(uint8_t* buffer,int xSz =1280*4,int ySz =720)
// {
//     for(int i =0;i<ySz;i++){
//         unsigned int offs = i*this->pitch;
//         Mmemcpy((this->fb + offs),(buffer + (i*xSz)),xSz);
//     }
// }

// void FrameBuffer::bufferCpy(uint8_t* buffer,long n)
// {
//     Mmemcpy(this->fb,buffer,n);
// }

// void FrameBuffer::drawChar(unsigned char ch, int x, int y, unsigned char attr)
// {
//     unsigned char *glyph = (unsigned char *)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

//     for (int i=0;i<FONT_HEIGHT;i++) {
// 	for (int j=0;j<FONT_WIDTH;j++) {
// 	    unsigned char mask = 1 << j;
// 	    unsigned char col = (*glyph & mask) ? attr & 0x0f : (attr & 0xf0) >> 4;

// 	    drawPixel(x+j, y+i, col);
// 	}
// 	glyph += FONT_BPL;
//     }
// }

// void FrameBuffer::drawString(int x, int y, char *s, unsigned char attr)
// {
//     while (*s) {
//        if (*s == '\r') {
//           x = 0;
//        } else if(*s == '\n') {
//           x = 0; y += FONT_HEIGHT;
//        } else {
// 	  drawChar(*s, x, y, attr);
//           x += FONT_WIDTH;
//        }
//        s++;
//     }
// }
