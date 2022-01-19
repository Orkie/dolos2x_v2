#include <dolos2x.h>
#include <gp2xregs.h>

static volatile uint16_t rGPIOCPINLVL;
static volatile uint16_t rGPIODPINLVL;
static volatile uint16_t rGPIOMPINLVL;

static void GPIOCPINLVL_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rGPIOCPINLVL;
}

static void GPIODPINLVL_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rGPIODPINLVL;
}

static void GPIOMPINLVL_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rGPIOMPINLVL;
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback) {
  rGPIOCPINLVL = 0xFFFF;
  rGPIODPINLVL = 0xFFFF;	
  rGPIOMPINLVL = 0xFFFF;	

  add_read_callback(REG(GPIOCPINLVL), GPIOCPINLVL_read);
  add_read_callback(REG(GPIODPINLVL), GPIODPINLVL_read);
  add_read_callback(REG(GPIOMPINLVL), GPIOMPINLVL_read);

  return 0;
}

void toggle(volatile uint16_t* reg, int bit, bool isUp) {
  *reg = isUp ? (*reg | BIT(bit)) : (CLEARBITS(*reg, BIT(bit)));
}

void toggleKey(SDL_Keycode key, bool isUp) {
  switch(key) {
  case SDLK_ESCAPE:
    exit(0);
    break;
  case SDLK_UP:
    toggle(&rGPIOMPINLVL, 0, isUp);
    break;
  case SDLK_LEFT:
    toggle(&rGPIOMPINLVL, 2, isUp);
    break;
  case SDLK_DOWN:
    toggle(&rGPIOMPINLVL, 4, isUp);
    break;
  case SDLK_RIGHT:
    toggle(&rGPIOMPINLVL, 6, isUp);
    break;
  case SDLK_s: // stick
    toggle(&rGPIODPINLVL, 11, isUp);
    break;
  case SDLK_LEFTBRACKET: // voldown
    toggle(&rGPIODPINLVL, 6, isUp);
    break;
  case SDLK_RIGHTBRACKET: // volup
    toggle(&rGPIODPINLVL, 7, isUp);
    break;
  case SDLK_l:
    toggle(&rGPIOCPINLVL, 10, isUp);
    break;
  case SDLK_r:
    toggle(&rGPIOCPINLVL, 11, isUp);
    break;
  case SDLK_y:
    toggle(&rGPIOCPINLVL, 15, isUp);
    break;
  case SDLK_a:
    toggle(&rGPIOCPINLVL, 12, isUp);
    break;
  case SDLK_b:
    toggle(&rGPIOCPINLVL, 13, isUp);
    break;
  case SDLK_x:
    toggle(&rGPIOCPINLVL, 14, isUp);
    break;    
  }
}

static void tick(void) {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    if(event.type == SDL_QUIT) {
      exit(0);
    } else if(event.type == SDL_KEYUP) {
      toggleKey(event.key.keysym.sym, true);
    } else if(event.type == SDL_KEYDOWN) {
      toggleKey(event.key.keysym.sym, false);	
    }
  }
}

dolos_peripheral peri_gpio = {
  .name = "GPIO",
  .init = &init,
  .tick = &tick
};
