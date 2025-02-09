#pragma once
#include <stdint.h>
#include "ARMMailbox.h"

class FrameBuffer {
    public:
        enum {
            MBOX_REQUEST  = 0
        };
        
        enum {
            MBOX_CH_POWER = 0,
            MBOX_CH_FB    = 1,
            MBOX_CH_VUART = 2,
            MBOX_CH_VCHIQ = 3,
            MBOX_CH_LEDS  = 4,
            MBOX_CH_BTNS  = 5,
            MBOX_CH_TOUCH = 6,
            MBOX_CH_COUNT = 7,
            MBOX_CH_PROP  = 8 // Request from ARM for response by VideoCore
        };
        
        enum {
            MBOX_TAG_SETPOWER   = 0x28001,
            MBOX_TAG_SETCLKRATE = 0x38002,
        
            MBOX_TAG_SETPHYWH   = 0x48003,
            MBOX_TAG_SETVIRTWH  = 0x48004,
            MBOX_TAG_SETVIRTOFF = 0x48009,
            MBOX_TAG_SETDEPTH   = 0x48005,
            MBOX_TAG_SETPXLORDR = 0x48006,
            MBOX_TAG_GETFB      = 0x40001,
            MBOX_TAG_GETPITCH   = 0x40008,
            MBOX_TAG_LAST       = 0
        };

        FrameBuffer();
        void drawByLine(uint8_t* buffer,int xSz =1280*4,int ySz =720);
        void bufferCpy(uint8_t* buffer,long n);
        void drawPixel(int x, int y, unsigned char attr);
        void drawPixelRGB(int x, int y, unsigned int colourRGB);
        void drawChar(unsigned char ch, int x, int y, unsigned char attr);
        void drawString(int x, int y, char *s, unsigned char attr);
        unsigned char * getFb();
        int getXYOffset(int x,int y,);
    private:
        static ARMMailbox FB_mailbox; //want only one instance even is somehow multiple fb get made;
        unsigned int width, height, pitch, isrgb; //7680 pitch
        unsigned char *fb;
};
