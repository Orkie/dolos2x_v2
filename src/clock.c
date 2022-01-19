#include <stdio.h>
#include <dolos2x.h>
#include <gp2xregs.h>

static uint16_t rFPLLVSETREG = 0x0;
static uint16_t rUPLLVSETREG = 0x0;
static uint16_t rAPLLVSETREG = 0x0;
static uint16_t rMEMCFGW = 0x0;

static void FPLLSETVREG_write(uint32_t addr, int bytes, void* value) {
  rFPLLVSETREG = U16_VAL(value);
}

static void FPLLVSETREG_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rFPLLVSETREG;
}

static void UPLLSETVREG_write(uint32_t addr, int bytes, void* value) {
  rUPLLVSETREG = U16_VAL(value);
}

static void UPLLVSETREG_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rUPLLVSETREG;
}

static void APLLSETVREG_write(uint32_t addr, int bytes, void* value) {
  rAPLLVSETREG = U16_VAL(value);
}

static void APLLVSETREG_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rAPLLVSETREG;
}

// we don't have the real delay updating the clock regs, so no need for this register to do anything
static void CLKCHGSTREG_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = 0x0;
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback) {
  add_read_callback(REG(CLKCHGSTREG), CLKCHGSTREG_read);
  
  add_read_callback(REG(FPLLVSETREG), FPLLVSETREG_read);
  add_write_callback(REG(FPLLSETVREG), FPLLSETVREG_write);

  add_read_callback(REG(UPLLVSETREG), UPLLVSETREG_read);
  add_write_callback(REG(UPLLSETVREG), UPLLSETVREG_write);
  
  add_read_callback(REG(APLLVSETREG), APLLVSETREG_read);
  add_write_callback(REG(APLLSETVREG), APLLSETVREG_write);
  
  return 0;
}

static void tick(void) {}

dolos_peripheral peri_clock = {
  .name = "Clock subsystem",
  .init = &init,
  .tick = &tick
};


