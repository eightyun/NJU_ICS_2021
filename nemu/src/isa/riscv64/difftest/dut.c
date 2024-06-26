#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"

// 进行寄存器状态的对比
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
  int reg_num = ARRLEN(cpu.gpr);
    for (int i = 0; i < reg_num; i++) 
    {
      if (ref_r->gpr[i]._64 != cpu.gpr[i]._64) 
      {
        return false;
      }
    }
    if (ref_r->pc != cpu.pc) 
    {
      return false;
    }
  return true;
}

void isa_difftest_attach() {
}
