#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <SDL.h>
#include <cpu.h>
#include <instr.h>

static struct termios old_tio, new_tio;

#define GP2X_RAM_SIZE (64 * 1024 * 1024)

void* gp2x_ram;
void* gp2x_mmio;

// TODO - when arm940 is added, these need to block on non-IO (i.e. RAM) accesses while the other core has the bus
// TODO - callback notifies upon read or write

int bus_fetch(uint32_t addr, int bytes, void* ret) {
  if(addr < GP2X_RAM_SIZE) {
    if(bytes == 4) {
      *((uint32_t*)ret) = ((uint32_t*)gp2x_ram)[addr>>2];
    } else if(bytes == 2) {
      *((uint16_t*)ret) = ((uint16_t*)gp2x_ram)[addr>>1];
    } else {
      *((uint8_t*)ret) = ((uint8_t*)gp2x_ram)[addr];
    }
    return 0;
  } else {
    fprintf(stderr, "Tried to write unmapped address 0x%x\n", addr);
    return -1;
  }
}

int bus_write(uint32_t addr, int bytes, void* value) {
  if(addr < GP2X_RAM_SIZE) {
    if(bytes == 4) {
      ((uint32_t*)gp2x_ram)[addr>>2] = *((uint32_t*)value);
    } else if(bytes == 2) {
      ((uint16_t*)gp2x_ram)[addr>>1] = *((uint16_t*)value);
    } else {
      ((uint8_t*)gp2x_ram)[addr] = *((uint8_t*)value);
    }
    return 0;
  } else {
    fprintf(stderr, "Tried to write unmapped address 0x%x\n", addr);
    return -1;
  }
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
  
  gp2x_ram = calloc(1, GP2X_RAM_SIZE);
  gp2x_mmio = calloc(1, 0x10000);
  
  pt_arm_cpu arm920;
  arm920.logging = true;
  pt_arm_init_cpu(&arm920, ARM920T,
		  &bus_fetch, &bus_write);

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
  while((c = fgetc(stdin)) != 'q') {
    clock(&arm920);
  }
  
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
  printf("\n");

  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(sdlWindow);
  SDL_Quit();
}

