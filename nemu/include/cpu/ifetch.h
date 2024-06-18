#ifndef __CPU_IFETCH_H__

#include <memory/vaddr.h>

// 专门负责取指令的工作
// 最后还会根据len来更新s->snpc, 从而让s->snpc指向下一条指令.
static inline uint32_t instr_fetch(vaddr_t *pc, int len) 
{  
  uint32_t instr = vaddr_ifetch(*pc, len);
  (*pc) += len;
  return instr;
}

#endif
