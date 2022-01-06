#include <stdio.h>
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
}

