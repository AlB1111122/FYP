#pragma once
#include <stdint.h>
#include "ARMMailbox.h"

class FrameBuffer {
    public:
        FrameBuffer();
        void drawByLine(uint8_t* buffer,int xSz =1280*4,int ySz =720);
        void bufferCpy(uint8_t* buffer,long n);
        void drawPixel(int x, int y, unsigned char attr);
        void drawPixelRGB(int x, int y, unsigned int colourRGB);
        void drawChar(unsigned char ch, int x, int y, unsigned char attr);
        void drawString(int x, int y, char *s, unsigned char attr);
    private:
        const uint32_t MBOX_REQUEST = 0;
        const uint32_t REQ_CHANNEL = 8;// Request from ARM for response by VideoCore

        class VCTag {
            public:
                static constexpr uint32_t MBOX_TAG_SETPOWER   = 0x28001;
                static constexpr uint32_t MBOX_TAG_SETCLKRATE = 0x38002;
            
                static constexpr uint32_t MBOX_TAG_SET_PHY_WH   = 0x48003;
                static constexpr uint32_t MBOX_TAG_SET_VIRT_WH  = 0x48004;
                static constexpr uint32_t MBOX_TAG_SET_VIRT_OFF = 0x48009;
                static constexpr uint32_t MBOX_TAG_SET_DEPTH   = 0x48005;
                static constexpr uint32_t MBOX_TAG_SET_PXL_ORDR = 0x48006;
                static constexpr uint32_t MBOX_TAG_GET_FB      = 0x40001;
                static constexpr uint32_t MBOX_TAG_GET_PITCH   = 0x40008;
                static constexpr uint32_t MBOX_TAG_LAST       = 0;
        };

    
        static constexpr VCTag VCTag{};
        static ARMMailbox FB_mailbox; //want only one instance even is somehow multiple fb get made;
        unsigned int width, height, pitch, isrgb; //7680 pitch
        unsigned char *fb;
        int getXYOffset(int x, int y);
        struct fb_mailbox_req_t {
            uint32_t size;
            uint32_t request;
            uint32_t set_phy_wh_tag;
            uint32_t set_phy_wh_size;
            uint32_t set_phy_wh_value;
            uint32_t width;
            uint32_t height;
            uint32_t set_virt_wh_tag;
            uint32_t set_virt_wh_size;
            uint32_t set_virt_wh_value;
            uint32_t virt_width;
            uint32_t virt_height;
            uint32_t set_virt_off_tag;
            uint32_t set_virt_off_size;
            uint32_t set_virt_off_value;
            uint32_t x_offset;
            uint32_t y_offset;
            uint32_t set_depth_tag;
            uint32_t set_depth_size;
            uint32_t set_depth_value;
            uint32_t depth;
            uint32_t set_pixel_order_tag;
            uint32_t set_pixel_order_size;
            uint32_t set_pixel_order_value;
            uint32_t pixel_order;
            uint32_t get_fb_tag;
            uint32_t get_fb_size;
            uint32_t get_fb_value;
            uint32_t fb_pointer;
            uint32_t fb_size;
            uint32_t get_pitch_tag;
            uint32_t get_pitch_size;
            uint32_t get_pitch_value;
            uint32_t pitch;
            uint32_t end_tag;
        } __attribute__((aligned(16)));
};
