#include <stdio.h>
#include <SDL.h>
#include <cpu.h>
#include <instr.h>

void* gp2x_ram;

uint32_t bus_fetch_word(uint32_t addr) {
  return ((uint32_t*)gp2x_ram)[addr>>2];
}

uint16_t bus_fetch_halfword(uint32_t addr) {
  return ((uint16_t*)gp2x_ram)[addr>>1];
}

uint8_t bus_fetch_byte(uint32_t addr) {
  return ((uint8_t*)gp2x_ram)[addr];
}

void bus_write_word(uint32_t addr, uint32_t value) {
  ((uint32_t*)gp2x_ram)[addr>>2] = value;
}

void bus_write_halfword(uint32_t addr, uint16_t value) {
  ((uint16_t*)gp2x_ram)[addr>>1] = value;
}

void bus_write_byte(uint32_t addr, uint8_t value) {
  ((uint8_t*)gp2x_ram)[addr] = value;
}

int main() {
  gp2x_ram = malloc(64 * 1024 * 1024);
  
  pt_arm_cpu cpu;
  pt_arm_init_cpu(&cpu, ARM920T,
		  &bus_fetch_word, &bus_fetch_halfword, &bus_fetch_byte,
		  &bus_write_word, &bus_write_halfword, &bus_write_byte);



  pt_arm_instruction instr;
  instr.type = INSTR_SINGLE_DATA_TRANSFER;
  instr.cond = COND_AL;
  instr.instr.single_data_transfer.add_offset_before_transfer = true;
  instr.instr.single_data_transfer.add_offset = true;
  instr.instr.single_data_transfer.transfer_byte = true;
  instr.instr.single_data_transfer.write_back_address = false;
  instr.instr.single_data_transfer.load = false;
  instr.instr.single_data_transfer.base = REG_R1;
  instr.instr.single_data_transfer.source_dest = REG_R0;
  instr.instr.single_data_transfer.offset.is_immediate = true;
  instr.instr.single_data_transfer.offset.op.imm.value = 0;
  cpu.r0 = 0x12345678;
  cpu.r1 = 0x2;

  printf("0x2: 0x%x\n", bus_fetch_byte(0x2));
  printf("0x0: 0x%.8x\n", bus_fetch_word(0x0));
  
  pt_arm_execute_instruction(&cpu, &instr);

  printf("0x2: 0x%x\n", bus_fetch_byte(0x2));
  printf("0x0: 0x%.8x\n", bus_fetch_word(0x0));
  
  SDL_Window* sdlWindow = SDL_CreateWindow("dolos2x", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, 0);
  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(sdlWindow);
  SDL_Quit();
}

