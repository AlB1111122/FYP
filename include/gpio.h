#pragma once
#include "./peripheral_reg.h"
class Gpio {
 public:
  void mmio_write(long reg, unsigned int val);
  unsigned int mmio_read(long reg);

  void gpio_useAsAlt3(unsigned int pin_number);
  void gpio_useAsAlt5(unsigned int pin_number);

 private:
  enum {
    GPFSEL0 = reg::PERIPHERAL_BASE + 0x200000,
    GPSET0 = reg::PERIPHERAL_BASE + 0x20001C,
    GPCLR0 = reg::PERIPHERAL_BASE + 0x200028,
    GPPUPPDN0 = reg::PERIPHERAL_BASE + 0x2000E4
  };

  enum {
    GPIO_MAX_PIN = 53,
    GPIO_FUNCTION_OUT = 1,
    GPIO_FUNCTION_ALT5 = 2,
    GPIO_FUNCTION_ALT3 = 7
  };

  enum { Pull_None = 0, Pull_Down = 2, Pull_Up = 1 };

  unsigned int gpio_call(unsigned int pin_number, unsigned int value,
                         unsigned int base, unsigned int field_size,
                         unsigned int field_max);

  unsigned int gpio_set(unsigned int pin_number, unsigned int value);
  unsigned int gpio_clear(unsigned int pin_number, unsigned int value);
  unsigned int gpio_pull(unsigned int pin_number, unsigned int value);
  unsigned int gpio_function(unsigned int pin_number, unsigned int value);

  void gpio_initOutputPinWithPullNone(unsigned int pin_number);

  void gpio_setPinOutputBool(unsigned int pin_number, unsigned int onOrOff);
};