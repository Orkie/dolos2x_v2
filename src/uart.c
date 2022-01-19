#include <dolos2x.h>
#include <gp2xregs.h>
#include <stdio.h>

static uint16_t rFSTATUS0 = 0x0;
static uint8_t rRHB0 = 0x0;
static int fifoCount = 0;
static uint8_t fifo[16];

static void THB0_write(uint32_t addr, int bytes, void* value) {
  printf("%c", (char) U8_VAL(value));
  fflush(stdout);
}

static void RHB0_read(uint32_t addr, int bytes, void* value) {
  U8_VAL(value) = 0x0;
}

static void FSTATUS0_read(uint32_t addr, int bytes, void* value) {
  U16_VAL(value) = 0x0;
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback) {
  add_read_callback(REG(RHB0), RHB0_read);
  add_read_callback(REG(FSTATUS0), FSTATUS0_read);
  add_write_callback(REG(THB0), THB0_write);

  return 0;
}

static void tick(void) {}

dolos_peripheral peri_uart = {
  .name = "UART",
  .init = &init,
  .tick = &tick
};

