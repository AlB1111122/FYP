#include "../../../include/FrameBuffer.h"

#include <stdlib.h>
#include <string.h>

#include "../../../include/gpio.h"
#include "../../../include/memCtrl.h"
#include "../../../include/terminal.h"

ARMMailbox FrameBuffer::FB_mailbox;

FrameBuffer::FrameBuffer() {
  // cleanInvalidateCache(FB_mailbox.mbox, fbSize);
  this->secondBuffer = false;
  // to make the code more readable
  FbMailboxReq_t* mbox =
      reinterpret_cast<volatile FbMailboxReq_t*>(FB_mailbox.mbox);
  mbox->size = 35 * 4;  // Length of message in bytes
  mbox->request = MBOX_REQUEST;

  mbox->setPhyWhTag = this->VCTag.MBOX_TAG_SET_PHY_WH;
  mbox->setPhyWhSize = 8;
  mbox->setPhyWhValue = 0;
  mbox->width = 1280;
  mbox->height = 720;

  mbox->setVirtWhTag = this->VCTag.MBOX_TAG_SET_VIRT_WH;
  mbox->setVirtWhSize = 8;
  mbox->setVirtWhValue = 8;
  mbox->virtWidth = 1280;
  mbox->virtHeight = 720;

  mbox->setVirtOffTag = this->VCTag.MBOX_TAG_SET_VIRT_OFF;
  mbox->setVirtOffSize = 8;
  mbox->setVirtOffValue = 8;
  mbox->xOffset = 0;
  mbox->yOffset = 0;

  mbox->setDepthTag = this->VCTag.MBOX_TAG_SET_DEPTH;
  mbox->setDepthSize = 4;
  mbox->setDepthValue = 4;
  mbox->depth = 32;

  mbox->setPixelOrderTag = this->VCTag.MBOX_TAG_SET_PXL_ORDR;
  mbox->setPixelOrderSize = 4;
  mbox->setPixelOrderValue = 4;
  mbox->pixelOrder = 1;

  mbox->getFbTag = this->VCTag.MBOX_TAG_GET_FB;
  mbox->getFbSize = 8;
  mbox->getFbValue = 8;
  mbox->fbPointer = 4096;
  mbox->fbSize = 0;

  mbox->getPitchTag = this->VCTag.MBOX_TAG_GET_PITCH;
  mbox->getPitchSize = 4;
  mbox->getPitchValue = 4;
  mbox->pitch = 0;

  mbox->endTag = this->VCTag.MBOX_TAG_LAST;

  // Check call is successful and we have a pointer with depth 32
  if (FB_mailbox.writeRead(REQ_CHANNEL) && mbox->depth == 32 &&
      mbox->fbPointer != 0) {
    mbox->fbPointer &= reg::GPU_TO_ARM_ADR_MASK;
    this->width = mbox->virtWidth;
    this->height = mbox->virtHeight;
    this->pitch = mbox->pitch;
    this->isrgb = mbox->pixelOrder;
    this->baseFb =
        reinterpret_cast<unsigned char*>(static_cast<long>(mbox->fbPointer));
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
    memcpy((this->fb + offs), (buffer + (i * xSz)), xSz);
  }
}

void FrameBuffer::bufferCpy(uint8_t* buffer) {
  long unsigned fbSize = this->width * this->height * 4;
  memcpy(this->fb, buffer, fbSize);
  // cleanInvalidateCache(this->fb, fbSize);
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
void FrameBuffer::pixelByPixelDraw(int xSrc, int ySrc, uint8_t* src) {
  int i = 1;
  for (int y = 0; y < ySrc; y++) {
    for (int x = 0; x < xSrc; x++) {
      this->drawPixelRGB(x, y,
                         ((src[i - 1] << 16) | (src[i] << 8) | src[i + 1]));
      i += 3;
    }
  }
}

void FrameBuffer::swapFb() {
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
