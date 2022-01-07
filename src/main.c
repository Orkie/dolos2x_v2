#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <SDL.h>
#include <cpu.h>
#include <instr.h>

static struct termios old_tio, new_tio;

#define GP2X_RAM_SIZE (64 * 1024 * 1024)

void* gp2x_ram;

uint32_t bus_fetch_word(uint32_t addr) {
  if(addr >= GP2X_RAM_SIZE) { fprintf(stderr, "Tried to read 0x%x\n", addr); }
  return ((uint32_t*)gp2x_ram)[addr>>2];
}

uint16_t bus_fetch_halfword(uint32_t addr) {
  if(addr >= GP2X_RAM_SIZE) { fprintf(stderr, "Tried to read 0x%x\n", addr); }
  return ((uint16_t*)gp2x_ram)[addr>>1];
}

uint8_t bus_fetch_byte(uint32_t addr) {
  if(addr >= GP2X_RAM_SIZE) { fprintf(stderr, "Tried to read 0x%x\n", addr); }
  return ((uint8_t*)gp2x_ram)[addr];
}

void bus_write_word(uint32_t addr, uint32_t value) {
  if(addr >= GP2X_RAM_SIZE) { fprintf(stderr, "Tried to read 0x%x 0x%x\n", addr, value); }
  ((uint32_t*)gp2x_ram)[addr>>2] = value;
}

void bus_write_halfword(uint32_t addr, uint16_t value) {
  if(addr >= GP2X_RAM_SIZE) { fprintf(stderr, "Tried to write 0x%x 0x%x\n", addr, value); }
  ((uint16_t*)gp2x_ram)[addr>>1] = value;
}

void bus_write_byte(uint32_t addr, uint8_t value) {
  if(addr >= GP2X_RAM_SIZE) { fprintf(stderr, "Tried to read 0x%x 0x%x\n", addr, value); }
  ((uint8_t*)gp2x_ram)[addr] = value;
}

void clock(pt_arm_cpu* arm920) {
  int r = pt_arm_clock(arm920);
  printf("Clock result: %d, R0: 0x%.8x R1: 0x%.8x\n\n", r, arm920->r0, arm920->r1);
}

int main() {
  tcgetattr(STDIN_FILENO, &old_tio);
  new_tio = old_tio;
  tcsetattr(STDIN_FILENO,TCSANOW, &old_tio);
  new_tio.c_lflag &=(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
  
  gp2x_ram = malloc(GP2X_RAM_SIZE);
  
  pt_arm_cpu arm920;
  arm920.logging = true;
  pt_arm_init_cpu(&arm920, ARM920T,
		  &bus_fetch_word, &bus_fetch_halfword, &bus_fetch_byte,
		  &bus_write_word, &bus_write_halfword, &bus_write_byte);

  FILE* bootloader = fopen("data/gp2xboot.img", "rb+");
  if(bootloader == NULL) {
    fprintf(stderr, "Error opening bootloader image\n");
    return 1;
  }

  if(fread(gp2x_ram, 1, 512, bootloader) != 512) {
    fprintf(stderr, "Could not load 512 bytes from bootloader image\n");
    return 1;
  }
      
  SDL_Window* sdlWindow = SDL_CreateWindow("dolos2x", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, 0);
  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

  char c = fgetc(stdin);
  while(c != 'q') {
    clock(&arm920);
  }

  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(sdlWindow);
  SDL_Quit();
}

