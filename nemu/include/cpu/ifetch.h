#ifndef __CPU_IFETCH_H__

#include <memory/vaddr.h>

static inline uint32_t instr_fetch(vaddr_t *pc, int len) {  // 专门负责取指令的工作
  uint32_t instr = vaddr_ifetch(*pc, len);
  (*pc) += len;
  return instr;
}

#endif
