#include "gpio.h"

unsigned int GPIO::gpioCall(unsigned int pinNumber, unsigned int value,
                            unsigned int base, unsigned int fieldSz,
                            unsigned int fieldMax) {
  // mask the other bits of the register
  unsigned int fieldMask = (1 << fieldSz) - 1;

  if (pinNumber > fieldMax || value > fieldMask) {
    return 0;
  }

  // 32 bit registers on this hardware
  // gpio reges controll multipule pins calc n pins conrolled by this reg
  unsigned int nFields = 32 / fieldSz;
  // calc target pin register address
  unsigned int reg = base + ((pinNumber / nFields) * 4);
  // shift to get to the right bits of the register
  unsigned int shift = (pinNumber % nFields) * fieldSz;

  // save old state
  unsigned int curVal = this->mmio.read(reg);
  // clear target pins bits, leave the others
  curVal &= ~(fieldMask << shift);
  // instert the new value to for target bits
  curVal |= value << shift;
  this->mmio.write(reg, curVal);

  return 1;
}

unsigned int GPIO::pinSet(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPSET0, 1, GPIO_MAX_PIN);
}
unsigned int GPIO::pinClear(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPCLR0, 1, GPIO_MAX_PIN);
}
unsigned int GPIO::pinPull(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPPUPPDN0, 2, GPIO_MAX_PIN);
}
unsigned int GPIO::pinFunction(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPFSEL0, 3, GPIO_MAX_PIN);
}

void GPIO::pinAsAlt3(unsigned int pinNumber) {
  pinPull(pinNumber, Pull_None);
  pinFunction(pinNumber, pinFunction_ALT3);
}

void GPIO::pinAsAlt5(unsigned int pinNumber) {
  pinPull(pinNumber, Pull_None);
  pinFunction(pinNumber, pinFunction_ALT5);
}

void GPIO::pinInitOutputWithPullNone(unsigned int pinNumber) {
  pinPull(pinNumber, Pull_None);
  pinFunction(pinNumber, pinFunction_OUT);
}

void GPIO::pinSetPinOutputBool(unsigned int pinNumber, unsigned int onOrOff) {
  if (onOrOff) {
    pinSet(pinNumber, 1);
  } else {
    pinClear(pinNumber, 1);
  }
}