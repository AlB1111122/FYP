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
  this->mmio.write(AUX_MU_BAUD_REG, AUX_MU_BAUD(115200));
  // set 14 & 15 to their alternate RX & TX functions for miniuart
  this->gpio.pinAsAlt5(14);
  this->gpio.pinAsAlt5(15);
  // enable RX & TX (miniuart pins)
  this->mmio.write(AUX_MU_CNTL_REG, 3);
}

unsigned int MiniUart::isOutputQueueEmpty() {
  return uart_output_queue_read == uart_output_queue_write;
}

unsigned int MiniUart::isReadByteReady() {
  return this->mmio.read(AUX_MU_LSR_REG) & 0x01;
}
unsigned int MiniUart::isWriteByteReady() {
  return this->mmio.read(AUX_MU_LSR_REG) & 0x20;
}

unsigned char MiniUart::readByte() {
  while (!this->isReadByteReady())
    ;
  return (unsigned char)this->mmio.read(AUX_MU_IO_REG);
}

void MiniUart::writeByteBlockingActual(unsigned char ch) {
  while (!this->isWriteByteReady())
    ;
  this->mmio.write(AUX_MU_IO_REG, (unsigned int)ch);
}

void MiniUart::loadOutputFifo() {
  while (!this->isOutputQueueEmpty() && this->isWriteByteReady()) {
    this->writeByteBlockingActual(uart_output_queue[uart_output_queue_read]);
    uart_output_queue_read =
        (uart_output_queue_read + 1) & (UART_MAX_QUEUE - 1);  // Don't overrun
  }
}

void MiniUart::writeByteBlocking(unsigned char ch) {
  unsigned int next =
      (uart_output_queue_write + 1) & (UART_MAX_QUEUE - 1);  // Don't overrun

  while (next == uart_output_queue_read) {
    this->loadOutputFifo();
  }

  uart_output_queue[uart_output_queue_write] = ch;
  uart_output_queue_write = next;
  this->loadOutputFifo();
}

void MiniUart::writeText(const etl::string<STR_SZ>& buffer) {
  for (const char& c : buffer) {
    if (c == '\n') {
      this->writeByteBlocking('\r');
    }
    this->writeByteBlocking(c);
  }
  // force que to empty to prevent overwrites from next call to uart
  this->drainOutputQueue();
}

void MiniUart::drainOutputQueue() {
  while (!this->isOutputQueueEmpty()) this->loadOutputFifo();
}

void MiniUart::update() {
  this->loadOutputFifo();

  if (this->isReadByteReady()) {
    unsigned char ch = this->readByte();
    if (ch == '\r')
      this->writeText("\n");
    else
      this->writeByteBlocking(ch);
  }
}
