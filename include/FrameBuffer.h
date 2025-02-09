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

        struct fb_mailbox_req_t {
            unsigned int size;
            unsigned int request;
            unsigned int set_phy_wh_tag;
            unsigned int set_phy_wh_size;
            unsigned int set_phy_wh_value;
            unsigned int width;
            unsigned int height;
            unsigned int set_virt_wh_tag;
            unsigned int set_virt_wh_size;
            unsigned int set_virt_wh_value;
            unsigned int virt_width;
            unsigned int virt_height;
            unsigned int set_virt_off_tag;
            unsigned int set_virt_off_size;
            unsigned int set_virt_off_value;
            unsigned int x_offset;
            unsigned int y_offset;
            unsigned int set_depth_tag;
            unsigned int set_depth_size;
            unsigned int set_depth_value;
            unsigned int depth;
            unsigned int set_pixel_order_tag;
            unsigned int set_pixel_order_size;
            unsigned int set_pixel_order_value;
            unsigned int pixel_order;
            unsigned int get_fb_tag;
            unsigned int get_fb_size;
            unsigned int get_fb_value;
            unsigned int fb_pointer;
            unsigned int fb_size;
            unsigned int get_pitch_tag;
            unsigned int get_pitch_size;
            unsigned int get_pitch_value;
            unsigned int pitch;
            unsigned int end_tag;
        } __attribute__((aligned(16)));

        FrameBuffer();
        void drawByLine(uint8_t* buffer,int xSz =1280*4,int ySz =720);
        void bufferCpy(uint8_t* buffer,long n);
        void drawPixel(int x, int y, unsigned char attr);
        void drawPixelRGB(int x, int y, unsigned int colourRGB);
        void drawChar(unsigned char ch, int x, int y, unsigned char attr);
        void drawString(int x, int y, char *s, unsigned char attr);
        unsigned char * getFb();
        //int getXYOffset(int x,int y,);
    private:
        static ARMMailbox FB_mailbox; //want only one instance even is somehow multiple fb get made;
        unsigned int width, height, pitch, isrgb; //7680 pitch
        unsigned char *fb;
};
