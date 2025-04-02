#include "../../../include/gpio.h"

void Gpio::mmioWrite(long reg, unsigned int val) {
  *(volatile unsigned int *)reg = val;
}
unsigned int Gpio::mmioRead(long reg) { return *(volatile unsigned int *)reg; }

unsigned int Gpio::gpioCall(unsigned int pinNumber, unsigned int value,
                            unsigned int base, unsigned int fieldSz,
                            unsigned int fieldMax) {
  unsigned int field_mask = (1 << fieldSz) - 1;

  if (pinNumber > fieldMax) return 0;
  if (value > field_mask) return 0;

  unsigned int num_fields = 32 / fieldSz;
  unsigned int reg = base + ((pinNumber / num_fields) * 4);
  unsigned int shift = (pinNumber % num_fields) * fieldSz;

  unsigned int curval = mmioRead(reg);
  curval &= ~(field_mask << shift);
  curval |= value << shift;
  mmioWrite(reg, curval);

  return 1;
}

unsigned int Gpio::pinSet(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPSET0, 1, GPIO_MAX_PIN);
}
unsigned int Gpio::pinClear(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPCLR0, 1, GPIO_MAX_PIN);
}
unsigned int Gpio::pinPull(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPPUPPDN0, 2, GPIO_MAX_PIN);
}
unsigned int Gpio::pinFunction(unsigned int pinNumber, unsigned int value) {
  return gpioCall(pinNumber, value, GPFSEL0, 3, GPIO_MAX_PIN);
}

void Gpio::pinAsAlt3(unsigned int pinNumber) {
  pinPull(pinNumber, Pull_None);
  pinFunction(pinNumber, pinFunction_ALT3);
}

void Gpio::pinAsAlt5(unsigned int pinNumber) {
  pinPull(pinNumber, Pull_None);
  pinFunction(pinNumber, pinFunction_ALT5);
}

void Gpio::pinInitOutputWithPullNone(unsigned int pinNumber) {
  pinPull(pinNumber, Pull_None);
  pinFunction(pinNumber, pinFunction_OUT);
}

void Gpio::pinSetPinOutputBool(unsigned int pinNumber, unsigned int onOrOff) {
  if (onOrOff) {
    pinSet(pinNumber, 1);
  } else {
    pinClear(pinNumber, 1);
  }
}