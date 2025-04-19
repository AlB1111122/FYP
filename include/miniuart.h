/*https://github.com/babbleberry/rpi4-osdev/tree/master?tab=CC0-1.0-1-ov-file
 * refrenced*/
#pragma once
#include <etl/string.h>

#include "gpio.h"
#include "mmio.h"

constexpr int STR_SZ = 1020;

class MiniUart {
 public:
  MiniUart();
  void writeText(const etl::string<STR_SZ>& buffer);

 private:
  enum {
    AUX_BASE = reg::PERIPHERAL_BASE + 0x215000,
    AUX_IRQ = AUX_BASE,
    AUX_ENABLES = AUX_BASE + 4,
    AUX_MU_IO_REG = AUX_BASE + 64,
    AUX_MU_IER_REG = AUX_BASE + 68,
    AUX_MU_IIR_REG = AUX_BASE + 72,
    AUX_MU_LCR_REG = AUX_BASE + 76,
    AUX_MU_MCR_REG = AUX_BASE + 80,
    AUX_MU_LSR_REG = AUX_BASE + 84,
    AUX_MU_MSR_REG = AUX_BASE + 88,
    AUX_MU_SCRATCH = AUX_BASE + 92,
    AUX_MU_CNTL_REG = AUX_BASE + 96,
    AUX_MU_STAT_REG = AUX_BASE + 100,
    AUX_MU_BAUD_REG = AUX_BASE + 104,
    AUX_UART_CLOCK = 500000000,
    UART_MAX_QUEUE = 16 * 1024,
    TX_IS_CLEAR = 0x20
  };

  unsigned char outputRingBuffer[UART_MAX_QUEUE];
  unsigned int outputWriteIdx = 0;
  unsigned int outputReadIdx = 0;

  GPIO gpio;
  MMIO mmio;

  uint32_t calcBaudrate(long baud) const;

  bool isOutputQueueEmpty() const;

  bool canWrite();

  void hardwareWrite();

  void writeChar(unsigned char ch);

  void flushOutput();

  friend MiniUart& operator<<(MiniUart& uart, const char* text) {
    uart.writeText(etl::string<STR_SZ>(text));
    return uart;
  }
  friend MiniUart& operator<<(MiniUart& uart, const etl::string<STR_SZ>& text) {
    uart.writeText(text);
    return uart;
  }
};
