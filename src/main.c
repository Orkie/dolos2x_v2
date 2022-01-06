#include <stdio.h>
#include <SDL.h>
#include <cpu.h>
#include <instr.h>

int main() {
  pt_arm_cpu cpu;
  pt_arm_init_cpu(&cpu, ARM920T, NULL, NULL, NULL, NULL, NULL, NULL);
  bool carryValid;
  bool carry;

  pt_arm_operand2 op;
  op.is_immediate = true;
  op.op.imm.value = 0x123456;
  op.op.imm.carryValid = true;
  op.op.imm.carry = false;
  
  uint32_t result = _petraea_eval_operand2(&cpu, &op, &carryValid, &carry);
  printf("result: 0x%x\n", result);

  SDL_Window* sdlWindow = SDL_CreateWindow("dolos2x", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 960, 0);
  SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(sdlWindow);
  SDL_Quit();
}

