/*https://github.com/babbleberry/rpi4-osdev/tree/master?tab=CC0-1.0-1-ov-file
 * borrowed heavily*/
#pragma once
#include "./gpio.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"

constexpr int STR_SZ = 1020;

class MiniUart {
 public:
  MiniUart();
  void init();
  void writeText(etl::string<STR_SZ> buffer);

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
    UART_MAX_QUEUE = 16 * 1024
  };

#define AUX_MU_BAUD(baud) ((AUX_UART_CLOCK / (baud * 8)) - 1)

  unsigned char uart_output_queue[UART_MAX_QUEUE];
  unsigned int uart_output_queue_write = 0;
  unsigned int uart_output_queue_read = 0;

  Gpio gpio;
  unsigned int isOutputQueueEmpty();

  unsigned int isReadByteReady();
  unsigned int isWriteByteReady();

  unsigned char readByte();

  void writeByteBlockingActual(unsigned char ch);

  void loadOutputFifo();

  void writeByteBlocking(unsigned char ch);

  void drainOutputQueue();

  void update();
};
