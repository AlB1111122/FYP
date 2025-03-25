#include "../../../include/FrameBuffer.h"

#include <stdlib.h>

#include "../../../include/gpio.h"
#include "../../../include/memCtrl.h"
#include "../../../include/terminal.h"

ARMMailbox FrameBuffer::FB_mailbox;

FrameBuffer::FrameBuffer() {
  this->secondBuffer = false;
  // to make the code more readable
  fb_mailbox_req_t* mbox =
      reinterpret_cast<volatile fb_mailbox_req_t*>(FB_mailbox.mbox);
  mbox->size = 35 * 4;  // Length of message in bytes
  mbox->request = MBOX_REQUEST;

  mbox->set_phy_wh_tag = this->VCTag.MBOX_TAG_SET_PHY_WH;
  mbox->set_phy_wh_size = 8;
  mbox->set_phy_wh_value = 0;
  mbox->width = 1280;
  mbox->height = 720;

  mbox->set_virt_wh_tag = this->VCTag.MBOX_TAG_SET_VIRT_WH;
  mbox->set_virt_wh_size = 8;
  mbox->set_virt_wh_value = 8;
  mbox->virt_width = 1280;
  mbox->virt_height =
      720;  //  * 2double the height to functionally get 2 buffers

  mbox->set_virt_off_tag = this->VCTag.MBOX_TAG_SET_VIRT_OFF;
  mbox->set_virt_off_size = 8;
  mbox->set_virt_off_value = 8;
  mbox->x_offset = 0;
  mbox->y_offset = 0;

  mbox->set_depth_tag = this->VCTag.MBOX_TAG_SET_DEPTH;
  mbox->set_depth_size = 4;
  mbox->set_depth_value = 4;
  mbox->depth = 32;

  mbox->set_pixel_order_tag = this->VCTag.MBOX_TAG_SET_PXL_ORDR;
  mbox->set_pixel_order_size = 4;
  mbox->set_pixel_order_value = 4;
  mbox->pixel_order = 1;

  mbox->get_fb_tag = this->VCTag.MBOX_TAG_GET_FB;
  mbox->get_fb_size = 8;
  mbox->get_fb_value = 8;
  mbox->fb_pointer = 4096;
  mbox->fb_size = 0;

  mbox->get_pitch_tag = this->VCTag.MBOX_TAG_GET_PITCH;
  mbox->get_pitch_size = 4;
  mbox->get_pitch_value = 4;
  mbox->pitch = 0;

  mbox->end_tag = this->VCTag.MBOX_TAG_LAST;

  // Check call is successful and we have a pointer with depth 32
  if (FB_mailbox.writeRead(REQ_CHANNEL) && mbox->depth == 32 &&
      mbox->fb_pointer != 0) {
    mbox->fb_pointer &= reg::GPU_TO_ARM_ADR_MASK;
    this->width = mbox->virt_width;
    this->height = mbox->virt_height;
    this->pitch = mbox->pitch;
    this->isrgb = mbox->pixel_order;
    this->baseFb =
        reinterpret_cast<unsigned char*>(static_cast<long>(mbox->fb_pointer));
    this->fb = this->baseFb;
  }
}

unsigned int FrameBuffer::getPitch() { return this->pitch; }

unsigned int FrameBuffer::getHeight() { return (this->height); }

void FrameBuffer::drawPixel(int x, int y, unsigned char attr) {
  int offs = getXYOffset(x, y);
  *((unsigned int*)(this->fb + offs)) = vgapal[attr & 0x0f];
}

void FrameBuffer::drawPixelRGB(int x, int y, unsigned int colourRGB) {
  int offs = getXYOffset(x, y);
  *((unsigned int*)(this->fb + offs)) = colourRGB;
}

void FrameBuffer::drawByLine(uint8_t* buffer, int xSz, int ySz) {
  for (int i = 0; i < ySz; i++) {
    unsigned int offs = i * this->pitch;
    Mmemcpy((this->fb + offs), (buffer + (i * xSz)), xSz);
  }
}

void FrameBuffer::bufferCpy(uint8_t* buffer, long n) {
  Mmemcpy(this->fb, buffer, n);
}

void FrameBuffer::drawChar(unsigned char ch, int x, int y, unsigned char attr) {
  unsigned char* glyph =
      (unsigned char*)&font + (ch < FONT_NUMGLYPHS ? ch : 0) * FONT_BPG;

  for (int i = 0; i < FONT_HEIGHT; i++) {
    for (int j = 0; j < FONT_WIDTH; j++) {
      unsigned char mask = 1 << j;
      unsigned char col = (*glyph & mask) ? attr & 0x0f : (attr & 0xf0) >> 4;

      this->drawPixel(x + j, y + i, col);
    }
    glyph += FONT_BPL;
  }
}

void FrameBuffer::drawString(int x, int y, char* s, unsigned char attr) {
  while (*s) {
    if (*s == '\r') {
      x = 0;
    } else if (*s == '\n') {
      x = 0;
      y += FONT_HEIGHT;
    } else {
      this->drawChar(*s, x, y, attr);
      x += FONT_WIDTH;
    }
    s++;
  }
}

int FrameBuffer::getXYOffset(int x, int y) {
  return (y * this->pitch) + (x * 4);
}

// expects rgb
void FrameBuffer::pixelByPixelDraw(int x_src, int y_src, uint8_t* src) {
  int i = 1;
  for (int y = 0; y < y_src; y++) {
    for (int x = 0; x < x_src; x++) {
      this->drawPixelRGB(x, y,
                         ((src[i - 1] << 16) | (src[i] << 8) | src[i + 1]));
      i += 3;
    }
  }
}

void FrameBuffer::swapFb() {
  uint32_t vsync = 0x0004800E;
  __asm__ volatile("dmb sy" ::: "memory");
  __asm__ volatile("isb sy" ::: "memory");

  this->secondBuffer = !this->secondBuffer;

  int newOffset = (this->height / 2) * this->secondBuffer;
  this->FB_mailbox.mbox[0] = 8 * 4;  // Length of message in bytes
  this->FB_mailbox.mbox[1] = MBOX_REQUEST;
  this->FB_mailbox.mbox[2] = this->VCTag.MBOX_TAG_SET_VIRT_OFF;
  this->FB_mailbox.mbox[3] = 0;
  this->FB_mailbox.mbox[4] = 0;
  this->FB_mailbox.mbox[5] = 0;
  this->FB_mailbox.mbox[6] = newOffset;  // Value(y)
  this->FB_mailbox.mbox[7] = this->VCTag.MBOX_TAG_LAST;
  FB_mailbox.writeRead(REQ_CHANNEL);

  this->fb =
      this->baseFb + (this->secondBuffer * (this->height / 2) * this->pitch);
  __asm__ volatile("dmb sy" ::: "memory");
}

unsigned char* FrameBuffer::getOffFb() {
  return this->baseFb +
         ((!this->secondBuffer) * (this->height / 2) * this->pitch);
}

unsigned char* FrameBuffer::getFb() { return this->fb; }