#ifndef __DOLOS2X_H__
#define __DOLOS2X_H__
#include <stdint.h>
#include <SDL.h>

typedef void (*mem_callback)(uint32_t, int, void*);
typedef void (*add_mem_callback)(uint32_t, mem_callback);
typedef void (*add_reg_binding32)(uint32_t, volatile uint32_t*);
typedef void (*add_reg_binding16)(uint32_t, volatile uint16_t*);

typedef struct {
  int (*init)(add_mem_callback, add_mem_callback, add_reg_binding32, add_reg_binding16);
  void (*tick)(void);
  const char* name;
} dolos_peripheral;

extern dolos_peripheral peri_nand;
extern dolos_peripheral peri_clock;
extern dolos_peripheral peri_timer;
extern dolos_peripheral peri_uart;
extern dolos_peripheral peri_video;
extern dolos_peripheral peri_gpio;

extern SDL_Renderer* sdlRenderer;

extern void* gp2x_ram;

#define CLEARBITS(in, bits) (in & (~bits))

#define U8_VAL(v) (*((uint8_t*)v))
#define U16_VAL(v) (*((uint16_t*)v))
#define U32_VAL(v) (*((uint32_t*)v))

#endif

