#include <dolos2x.h>
#include <gp2xregs.h>

static volatile uint16_t rGPIOBPINLVL;
static volatile uint32_t rMLC_STL_ADDR;

static volatile uint32_t lastVsync;

static void GPIOBPINLVL_read(uint32_t addr, int bytes, void* ret) {
  U16_VAL(ret) = rGPIOBPINLVL;
}

static void MLC_STL_OADRL_write(uint32_t addr, int bytes, void* value) {
  rMLC_STL_ADDR = (rMLC_STL_ADDR&0xFFFF0000) | U16_VAL(value);
}

static void MLC_STL_OADRH_write(uint32_t addr, int bytes, void* value) {
  rMLC_STL_ADDR = (rMLC_STL_ADDR&0xFFFF) | (U16_VAL(value) << 16);
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback, add_reg_binding32 bind_reg32, add_reg_binding16 bind_reg16) {
  rGPIOBPINLVL = 0;

  lastVsync = SDL_GetTicks();

  bind_reg16(REG(GPIOBPINLVL), &rGPIOBPINLVL);
  add_write_callback(REG(MLC_STL_OADRL), MLC_STL_OADRL_write);
  add_write_callback(REG(MLC_STL_OADRH), MLC_STL_OADRH_write);
  
  return 0;
}

static void tick(void) {
  uint32_t currentTicks = SDL_GetTicks();
  if((currentTicks - lastVsync) >= 16) {
    // TODO - figure out correct timing for v/hsync on GP2X
    lastVsync = currentTicks;
    rGPIOBPINLVL |= BIT(4);
    SDL_Delay(40);
    rGPIOBPINLVL = CLEARBITS(rGPIOBPINLVL, BIT(4));
    
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom((gp2x_ram+rMLC_STL_ADDR), 320, 240, 16, 320*2, SDL_PIXELFORMAT_RGB565);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, texture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
  }
}

dolos_peripheral peri_video = {
  .name = "Video",
  .init = &init,
  .tick = &tick
};

