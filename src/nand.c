#include <dolos2x.h>
#include <gp2xregs.h>

#include <stdio.h>

static void MEMNANDCTRLW_write(uint32_t addr, int bytes, void* ret) {
  printf("writing %d bytes to 0x%.8x with value 0x%.8x\n", bytes, addr, *((uint16_t*)ret));
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback) {
  add_write_callback(REG(MEMNANDCTRLW), MEMNANDCTRLW_write);
  return 0;
}

dolos_peripheral peri_nand = {
  .name = "NAND",
  .init = &init
};

