#include <common.h>
#include <utils.h>
#include <stdlib.h>


#define MAX_IRINGBUF 128
#define ANSI_FG_RED "\x1b[31m"
#define ANSI_NONE "\x1b[0m"


typedef struct 
{
  word_t pc;        // 程序计数器值
  uint32_t inst;   // 指令值
} ItraceNode;

//------------------------------------iringbuf----------------------------------------------

ItraceNode iringbuf[MAX_IRINGBUF]; // 环形缓冲区数组
int p_cur = 0; // 当前位置指针
bool full = false; // 缓冲区是否已满的标志

void trace_inst(word_t pc, uint32_t inst) 
{
  iringbuf[p_cur].pc = pc;  // 将当前指令的PC值保存到缓冲区
  iringbuf[p_cur].inst = inst;  // 将当前指令的机器码保存到缓冲区
  p_cur = (p_cur + 1) % MAX_IRINGBUF;  // 更新缓冲区指针，循环使用缓冲区

  if (p_cur == 0) 
        full = true;  // 缓冲区已满
}

void display_inst() 
{
  if (!full && !p_cur) return;

  int end = p_cur; // 结束位置为当前位置指针
  int i = full?p_cur:0;   // 如果缓冲区已满，则从当前位置开始，否则从 0 开始

    //disassemble函数用于将指令解析成可读的格式  声明反汇编函数
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);  
  char buf[128]; 
  char *p;
  printf("Most recently executed instructions\n");
  do 
  {
    p = buf;
    p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i + 1)%MAX_IRINGBUF == end?" --> ":"     ", iringbuf[i].pc, iringbuf[i].inst);
    disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

    if ((i + 1)%MAX_IRINGBUF==end) printf(ANSI_FG_RED);
    puts(buf);
  } 
  while ((i = (i + 1) % MAX_IRINGBUF) != end);
  puts(ANSI_NONE);
}


//------------------------------------mtrace----------------------------------------------

void display_pread(paddr_t addr,int len)
{
    printf("pread at " FMT_PADDR " len = %d\n", addr, len);
}

void display_pwrite(paddr_t addr, int len, word_t data) 
{
  printf("pwrite at " FMT_PADDR " len = %d, data = " FMT_WORD "\n", addr, len, data);
}
