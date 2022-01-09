#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <SDL.h>
#include <glib.h>

#include <dolos2x.h>
#include <cpu.h>
#include <instr.h>

static struct termios old_tio, new_tio;

#define GP2X_RAM_SIZE (64 * 1024 * 1024)

static void* gp2x_ram;
static GHashTable* mmio_read_callbacks;
static GHashTable* mmio_write_callbacks;

static void add_read_callback(uint32_t addr, mem_callback cb) {
  g_hash_table_insert(mmio_read_callbacks, GUINT_TO_POINTER(addr), cb);
}

static void add_write_callback(uint32_t addr, mem_callback cb) {
  g_hash_table_insert(mmio_write_callbacks, GUINT_TO_POINTER(addr), cb);
}

// TODO - when arm940 is added, these need to block on non-IO (i.e. RAM) accesses while the other core has the bus
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
    void (*cb)(uint32_t, int, void*) = g_hash_table_lookup(mmio_read_callbacks, GUINT_TO_POINTER(addr));
    if(cb != NULL) {
      (*cb)(addr, bytes, ret);
      return 0;
    } else {
      fprintf(stderr, "Tried to read unmapped address 0x%x\n", addr);
      return -1;
    }
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
    void (*cb)(uint32_t, int, void*) = g_hash_table_lookup(mmio_write_callbacks, GUINT_TO_POINTER(addr));
    if(cb != NULL) {
      (*cb)(addr, bytes, value);
      return 0;
    } else {
      fprintf(stderr, "Tried to write unmapped address 0x%x\n", addr);
      return -1;
    }
  }
}

void clock_cpu(pt_arm_cpu* arm920) {
  int r = pt_arm_clock(arm920);
  printf("Clock result: %d, R0: 0x%.8x R1: 0x%.8x\n\n", r, arm920->r0, arm920->r1);
}

void cleanup() {
    //  SDL_DestroyRenderer(sdlRenderer);
  //  SDL_DestroyWindow(sdlWindow);
  //  SDL_Quit();

  g_hash_table_destroy(mmio_read_callbacks);
  g_hash_table_destroy(mmio_write_callbacks);
  free(gp2x_ram);
  
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
  printf("\n");
}

int main() {
  tcgetattr(STDIN_FILENO, &old_tio);
  new_tio = old_tio;
  tcsetattr(STDIN_FILENO,TCSANOW, &old_tio);
  new_tio.c_lflag &=(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
  
  gp2x_ram = calloc(1, GP2X_RAM_SIZE);
  mmio_read_callbacks = g_hash_table_new(g_direct_hash, g_direct_equal);
  mmio_write_callbacks = g_hash_table_new(g_direct_hash, g_direct_equal);

  atexit(cleanup);

  dolos_peripheral peripherals[] = {
    peri_nand
  };
  for(int i = 0 ; i < (sizeof(peripherals)/sizeof(dolos_peripheral)) ; i++) {
    dolos_peripheral p = peripherals[i];
    printf("Initialising %s\n", p.name);
    int r = p.init(&add_read_callback, &add_write_callback);
    if(r != 0) {
      fprintf(stderr, "Failed to initialise %s (%d), terminating!\n", p.name, r);
      exit(1);
    }
  }
    
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
      
  //  SDL_Window* sdlWindow = SDL_CreateWindow("dolos2x", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, 0);
  //  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

  char c = fgetc(stdin);
  while((c = fgetc(stdin)) != 'q') {
    clock_cpu(&arm920);
  }
}

