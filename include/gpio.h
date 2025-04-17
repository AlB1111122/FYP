/*https://github.com/babbleberry/rpi4-osdev/tree/master?tab=CC0-1.0-1-ov-file
 * based off*/
#pragma once
#include "./mmio.h"
#include "./peripheralReg.h"
class GPIO {
 public:
  void pinAsAlt3(unsigned int pinNumber);
  void pinAsAlt5(unsigned int pinNumber);

 private:
  MMIO mmio;
  enum {
    GPFSEL0 = reg::PERIPHERAL_BASE + 0x200000,
    GPSET0 = reg::PERIPHERAL_BASE + 0x20001C,
    GPCLR0 = reg::PERIPHERAL_BASE + 0x200028,
    GPPUPPDN0 = reg::PERIPHERAL_BASE + 0x2000E4
  };

  enum {
    GPIO_MAX_PIN = 53,
    pinFunction_OUT = 1,
    pinFunction_ALT5 = 2,
    pinFunction_ALT3 = 7
  };

  enum { Pull_None = 0, Pull_Down = 2, Pull_Up = 1 };

  unsigned int gpioCall(unsigned int pinNumber, unsigned int value,
                        unsigned int base, unsigned int fieldSz,
                        unsigned int fieldMax);

  unsigned int pinSet(unsigned int pinNumber, unsigned int value);
  unsigned int pinClear(unsigned int pinNumber, unsigned int value);
  unsigned int pinPull(unsigned int pinNumber, unsigned int value);
  unsigned int pinFunction(unsigned int pinNumber, unsigned int value);

  void pinInitOutputWithPullNone(unsigned int pinNumber);

  void pinSetPinOutputBool(unsigned int pinNumber, unsigned int onOrOff);
};