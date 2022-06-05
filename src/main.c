#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_net.h>
#include <glib.h>

#include <dolos2x.h>
#include <cpu.h>
#include <instr.h>

static struct termios old_tio, new_tio;

#define GP2X_RAM_SIZE (64 * 1024 * 1024)

void* gp2x_ram;
static GHashTable* mmio_read_callbacks;
static GHashTable* mmio_write_callbacks;

SDL_Renderer* sdlRenderer;
static SDL_Window* sdlWindow;
static SDL_Thread* peripheralsThread;
static SDL_Thread* arm920Thread;

static IPaddress ip;
static TCPsocket tcpsock;

static int nPeripherals;

//static mem_callback mmio_read_callbacks[];
//static mem_callback mmio_write_callbacks[];

static void add_read_callback(uint32_t addr, mem_callback cb) {
  g_hash_table_insert(mmio_read_callbacks, GUINT_TO_POINTER(addr), cb);
}

static void add_write_callback(uint32_t addr, mem_callback cb) {
  g_hash_table_insert(mmio_write_callbacks, GUINT_TO_POINTER(addr), cb);
}

typedef enum {
  AWAITING_START,
  READING_COMMAND,
  READING_CHECKSUM
} debugger_read_state;

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
      //      fprintf(stderr, "Tried to read unmapped address 0x%.8x\n", addr);
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
      //      fprintf(stderr, "Tried to write unmapped address 0x%.8x\n", addr);
      return -1;
    }
  }
}

void clock_cpu(pt_arm_cpu* arm920, bool log) {
  int r = pt_arm_clock(arm920);
  if(r != 0) {
    fprintf(stderr, "pt_arm_clock returned %d at 0x%.8x\n", r, arm920->r15);
  }
  if(log) {
    printf("PC: 0x%.8x R0: 0x%.8x R1: 0x%.8x R2: 0x%.8x R3: 0x%.8x R4: 0x%.8x R5: 0x%.8x\n\n", arm920->r15, arm920->r0, arm920->r1, arm920->r2, arm920->r3, arm920->r4, arm920->r5);
  }
}

int peripheralsThreadFn(void* data) {
  dolos_peripheral* peripherals = (dolos_peripheral*) data;
  while(true) {
    for(int i = 0 ; i < nPeripherals ; i++) {
      peripherals[i].tick();
    }
  }

  return 0;
}

int arm920ThreadFn(void* data) {
  pt_arm_cpu* arm920 = (pt_arm_cpu*) data;
  uint32_t start = SDL_GetTicks();
  long n;
  while(true){//arm920.r15 != 0xE8) {
    pt_arm_clock(arm920);
    n++;
    if(n > 220000000) {
      printf("Ran 220M instructions in %u ms\n", (SDL_GetTicks() - start));
      start = SDL_GetTicks();
      n = 0;
    }
    //    clock_cpu(&arm920, false);
  }

}

void cleanup() {
  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(sdlWindow);

  SDLNet_TCP_Close(tcpsock);
  
  SDLNet_Quit();
  SDL_Quit();

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

  SDL_Init(SDL_INIT_EVERYTHING);
  SDLNet_Init();

  if(SDLNet_ResolveHost(&ip, NULL, 2345) == -1) {
    printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
    exit(1);
  }

  tcpsock = SDLNet_TCP_Open(&ip);
  if(!tcpsock) {
    printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
    exit(2);
  }


  atexit(cleanup);

  dolos_peripheral peripherals[] = {
    peri_nand,
    peri_clock,
    peri_timer,
    peri_uart,
    peri_video,
    peri_gpio
  };

  nPeripherals = (sizeof(peripherals)/sizeof(dolos_peripheral));
  
  for(int i = 0 ; i < nPeripherals ; i++) {
    dolos_peripheral p = peripherals[i];
    printf("Initialising %s\n", p.name);
    int r = p.init(&add_read_callback, &add_write_callback);
    if(r != 0) {
      fprintf(stderr, "Failed to initialise %s (%d), terminating!\n", p.name, r);
      exit(1);
    }
  }
    
  pt_arm_cpu arm920;
  //  arm920.logging = true;
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
      
  sdlWindow = SDL_CreateWindow("dolos2x", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, 0);
  sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

  peripheralsThread = SDL_CreateThread(peripheralsThreadFn, "Peripherals Thread", peripherals);

  arm920Thread = SDL_CreateThread(arm920ThreadFn, "ARM920T Thread", &arm920);

  uint8_t debugBuffer[4096];
  unsigned int debugLength = 0;
  uint8_t debugChecksum = 0x0;

  uint8_t checksum(const char *buf, size_t len) {
	uint8_t csum;
	csum = 0;
	while (len--) {
	  csum += *buf++;
	}
	return csum;
  }

  char* hex = "0123456789abcdef";

  void debugSend(TCPsocket sock, const char* msg) {
    SDLNet_TCP_Send(sock, "+$", 2);

    char response[2048] = {0x0};
    int responseLength = 0;
    
    for(int b = 0 ; b < strlen(msg) ; b++) {
      response[responseLength++] = msg[b];
    }

    SDLNet_TCP_Send(sock, response, responseLength);

    uint8_t checksumByte = checksum(response, responseLength);
    char checksum[2] = {
      hex[((checksumByte>>4)&0xF)],
      hex[(checksumByte&0xF)]
    };

    SDLNet_TCP_Send(sock, "#", 1);
    SDLNet_TCP_Send(sock, checksum, 2);
  }
  
  while(true) {
    TCPsocket new_tcpsock = SDLNet_TCP_Accept(tcpsock);
    debugger_read_state state = AWAITING_START;
    if(new_tcpsock) {
      printf("GDB: Debugger connected\n");
      while(true) {
	uint8_t byte;
	if(SDLNet_TCP_Recv(new_tcpsock, &byte, 1) <= 0) {
	  break;
	}

	if(state == AWAITING_START && byte == '$') {
	  state = READING_COMMAND;
	} else if(state == READING_COMMAND && byte != '#') {
	  // TODO - check we aren't overflowing the buffer
	  debugBuffer[debugLength++] = byte;
	} else if(state == READING_COMMAND && byte == '#') {
	  state = READING_CHECKSUM;
	  debugBuffer[debugLength++] = 0x0;
	} else if(state == READING_CHECKSUM) {
	  debugChecksum = byte;
	  state = AWAITING_START;
	  debugLength = 0;
	  // TODO - verify checksum and run command
	  printf("GDB: Got command: %s\n", debugBuffer);
	  if(strcmp(debugBuffer, "?") == 0) {
	    printf("GDB: Replying SIGTRAP\n");
	    debugSend(new_tcpsock, "S50");
	  } else if(strcmp(debugBuffer, "g") == 0) {
	    printf("GDB: Reading registers\n");
	    pt_arm_registers* regs = pt_arm_get_regs(&arm920);
	    char output[201];
	    output[200] = 0x0;
	    int outputCount = 0;
	    for(int i = 0 ; i < 16 ; i++) {
	      uint32_t r = *regs->regs[i];
	      printf("returning reg 0x%x\n", r);
	      output[outputCount++] = hex[((r>>4)&0xF) % 16];
	      output[outputCount++] = hex[((r>>0)&0xF) % 16];

	      output[outputCount++] = hex[((r>>12)&0xF) % 16];
	      output[outputCount++] = hex[((r>>8)&0xF) % 16];

	      output[outputCount++] = hex[((r>>20)&0xF) % 16];
	      output[outputCount++] = hex[((r>>16)&0xF) % 16];
	      
	      output[outputCount++] = hex[((r>>28)&0xF) % 16];
	      output[outputCount++] = hex[((r>>24)&0xF) % 16];
	    }
	    for(int i = 0 ; i < 9 ; i++) {
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';
	    output[outputCount++] = 'x';

	    }
	    debugSend(new_tcpsock, output);
	  } else {
	    printf("GDB: Not sure what to do with this...\n");
	    debugSend(new_tcpsock, "");
	  }
	}
      }
      printf("\nGDB: Debugger disconnected\n");
    }
    SDL_Delay(200);
  }
}

