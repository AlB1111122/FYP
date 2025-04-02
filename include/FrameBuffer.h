#pragma once
#include <stdint.h>

#include "ARMMailbox.h"

class FrameBuffer {
 public:
  FrameBuffer();
  void drawByLine(uint8_t* buffer, int xSz = 1280 * 4, int ySz = 720);
  void bufferCpy(uint8_t* buffer);
  void drawPixel(int x, int y, unsigned char attr);
  void drawPixelRGB(int x, int y, unsigned int colourRGB);
  void drawChar(unsigned char ch, int x, int y, unsigned char attr);
  void drawString(int x, int y, char* s, unsigned char attr);
  void pixelByPixelDraw(int xSrc, int ySrc, uint8_t* src);
  unsigned int getPitch();
  unsigned int getHeight();
  void swapFb();
  unsigned char* getOffFb();
  unsigned char* getFb();

 private:
  const uint32_t MBOX_REQUEST = 0;
  const uint32_t REQ_CHANNEL = 8;  // Request from ARM for response by VideoCore

  class VCTag {
   public:
    static constexpr uint32_t MBOX_TAG_SETPOWER = 0x28001;
    static constexpr uint32_t MBOX_TAG_SETCLKRATE = 0x38002;

    static constexpr uint32_t MBOX_TAG_SET_PHY_WH = 0x48003;
    static constexpr uint32_t MBOX_TAG_SET_VIRT_WH = 0x48004;
    static constexpr uint32_t MBOX_TAG_SET_VIRT_OFF = 0x48009;
    static constexpr uint32_t MBOX_TAG_SET_DEPTH = 0x48005;
    static constexpr uint32_t MBOX_TAG_SET_PXL_ORDR = 0x48006;
    static constexpr uint32_t MBOX_TAG_GET_FB = 0x40001;
    static constexpr uint32_t MBOX_TAG_GET_PITCH = 0x40008;
    static constexpr uint32_t WAIT_FOR_VSYNC = 0x4800E;
    static constexpr uint32_t MBOX_TAG_LAST = 0;
  };

  bool secondBuffer = 0;
  static constexpr VCTag VCTag{};
  static ARMMailbox FB_mailbox;  // want only one instance even is somehow
                                 // multiple fb get made;
  unsigned int width, height, pitch, isrgb;  // 7680 pitch
  unsigned char* fb;
  unsigned char* baseFb;
  int getXYOffset(int x, int y);
  struct FbMailboxReq_t {
    uint32_t size;
    uint32_t request;

    uint32_t setPhyWhTag;
    uint32_t setPhyWhSize;
    uint32_t setPhyWhValue;
    uint32_t width;
    uint32_t height;

    uint32_t setVirtWhTag;
    uint32_t setVirtWhSize;
    uint32_t setVirtWhValue;
    uint32_t virtWidth;
    uint32_t virtHeight;

    uint32_t setVirtOffTag;
    uint32_t setVirtOffSize;
    uint32_t setVirtOffValue;
    uint32_t xOffset;
    uint32_t yOffset;

    uint32_t setDepthTag;
    uint32_t setDepthSize;
    uint32_t setDepthValue;
    uint32_t depth;

    uint32_t setPixelOrderTag;
    uint32_t setPixelOrderSize;
    uint32_t setPixelOrderValue;
    uint32_t pixelOrder;

    uint32_t getFbTag;
    uint32_t getFbSize;
    uint32_t getFbValue;
    uint32_t fbPointer;
    uint32_t fbSize;

    uint32_t getPitchTag;
    uint32_t getPitchSize;
    uint32_t getPitchValue;
    uint32_t pitch;

    uint32_t endTag;
  } __attribute__((aligned(16))) __attribute__((packed));
};
