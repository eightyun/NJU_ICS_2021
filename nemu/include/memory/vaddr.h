#ifndef __MEMORY_VADDR_H__
#define __MEMORY_VADDR_H__

#include <common.h>

word_t vaddr_ifetch(vaddr_t addr, int len);         // 对虚拟地址进行指令（instruction）的读取操作
word_t vaddr_read(vaddr_t addr, int len);           // 对虚拟地址进行内存读取操作
void vaddr_write(vaddr_t addr, int len, word_t data);       // 对虚拟地址进行内存写入操作

#define PAGE_SHIFT        12
#define PAGE_SIZE         (1ul << PAGE_SHIFT)
#define PAGE_MASK         (PAGE_SIZE - 1)

#endif
