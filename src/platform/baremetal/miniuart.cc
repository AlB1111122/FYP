#include "../../../include/miniuart.h"

#include "../../../include/peripheralReg.h"

MiniUart::MiniUart() {
  this->mmio.write(AUX_ENABLES, 1);      // enable UART1
  this->mmio.write(AUX_MU_IER_REG, 0);   // disable interupts
  this->mmio.write(AUX_MU_CNTL_REG, 0);  // disable uart recive and disable
  // 8 bits mode,(error in datasheet: says ony 0 matters but false need 3)
  this->mmio.write(AUX_MU_LCR_REG, 3);
  this->mmio.write(AUX_MU_MCR_REG, 0);  // UART1_RTS line to high
  //  enable recive interrupt, acess baudrate reg 11000110
  this->mmio.write(AUX_MU_IIR_REG, 0xC6);
  // set the baudrate to 115200
  this->mmio.write(AUX_MU_BAUD_REG, calcBaudrate(115200));
  // set 14 & 15 to their alternate RX & TX functions for miniuart
  this->gpio.pinAsAlt5(14);
  this->gpio.pinAsAlt5(15);
  // enable RX & TX (miniuart pins)
  this->mmio.write(AUX_MU_CNTL_REG, 3);
}

uint32_t MiniUart::calcBaudrate(long baud) const {
  return (AUX_UART_CLOCK / (baud * 8)) - 1;
}

bool MiniUart::isOutputQueueEmpty() const {
  return outputReadIdx == outputWriteIdx;
}

bool MiniUart::canWrite() {
  return this->mmio.read(AUX_MU_LSR_REG) & TX_IS_CLEAR;
}

void MiniUart::hardwareWrite() {
  while (!this->isOutputQueueEmpty() && this->canWrite()) {
    this->mmio.write(AUX_MU_IO_REG, outputRingBuffer[outputReadIdx]);
    // bitmask to deal with ring buffer wrap around
    outputReadIdx =
        (outputReadIdx + 1) & (UART_MAX_QUEUE - 1);  // Don't overrun
  }
}

void MiniUart::writeChar(unsigned char ch) {
  unsigned int next =
      (outputWriteIdx + 1) & (UART_MAX_QUEUE - 1);  // Don't overrun

  while (next == outputReadIdx) {
    // to keep the buffer from ever being too full
    this->hardwareWrite();
  }
  // add to the queue to be written
  outputRingBuffer[outputWriteIdx] = ch;
  outputWriteIdx = next;
  this->hardwareWrite();
}

void MiniUart::writeText(const etl::string<STR_SZ>& buffer) {
  for (const char& c : buffer) {
    // otherwise newlines causes output to look tabbed across terminal
    if (c == '\n') {
      this->writeChar('\r');
    }
    this->writeChar(c);
  }
  // force que to empty to prevent overwrites from next call to uart
  while (!this->isOutputQueueEmpty()) {
    this->hardwareWrite();
  }
}
