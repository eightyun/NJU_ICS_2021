#include <common.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stdarg.h>

#define MAX_IRINGBUF 64
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

//------------------------------------ftrace----------------------------------------------

// 记录函数调用和返回则在jal与jalr这两个指令执行时调用

//用于存储符号表中的行 
typedef struct 
{
	char name[32]; 
	paddr_t addr;
	unsigned char info;
	Elf64_Xword size;
} SymEntry;
SymEntry *symbol_tbl = NULL; 
int symbol_tbl_size = 0;
int call_depth = 0;

// 用于跟踪尾递归的调用的链表  维护在”等待返回“的函数调用，即被尾调用优化的函数调用
typedef struct tail_rec_node 
{
	paddr_t pc;
	paddr_t depend;
	struct tail_rec_node *next;
} TailRecNode;
TailRecNode *tail_rec_head = NULL; 

// 专门用于输出字符串
void ftrace_write(const char *format, ...) 
{
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
}


// 读取 ELF 文件头并进行基本验证
static void read_elf_header(int fd, Elf64_Ehdr *eh) 
{
  // 将文件指针重置到文件开始位置
    if (lseek(fd, 0, SEEK_SET) == -1) 
        panic("Failed to seek to the beginning of the file");
    
  // 从文件中读取 ELF 头部信息到 eh 指针指向的内存中
    ssize_t bytesRead = read(fd, (void *)eh, sizeof(Elf64_Ehdr));
    if (bytesRead != sizeof(Elf64_Ehdr)) 
    {
        if (bytesRead == -1) 
            panic("Failed to read ELF header");
         else 
            panic("Incomplete ELF header read");
    }

  // 检查文件是否为 ELF 文件
    if (strncmp((char*)eh->e_ident, "\177ELF", 4) != 0) 
        panic("malformed ELF file");
}

// 输出了 ELF 文件头的信息
static void display_elf_header(Elf64_Ehdr eh) 
{
    /* Storage capacity class */
    ftrace_write("Storage class\t= ");
    switch (eh.e_ident[EI_CLASS]) 
    {
        case ELFCLASS32:
            ftrace_write("32-bit objects\n");
            break;
        case ELFCLASS64:
            ftrace_write("64-bit objects\n");
            break;
        default:
            ftrace_write("INVALID CLASS\n");
            break;
    }

    /* Data Format */
    ftrace_write("Data format\t= ");
    switch (eh.e_ident[EI_DATA]) 
    {
        case ELFDATA2LSB:
            ftrace_write("2's complement, little endian\n");
            break;
        case ELFDATA2MSB:
            ftrace_write("2's complement, big endian\n");
            break;
        default:
            ftrace_write("INVALID Format\n");
            break;
    }

    /* OS ABI */
    ftrace_write("OS ABI\t\t= ");
    switch (eh.e_ident[EI_OSABI]) 
    {
        case ELFOSABI_SYSV:
            ftrace_write("UNIX System V ABI\n");
            break;
        case ELFOSABI_HPUX:
            ftrace_write("HP-UX\n");
            break;
        case ELFOSABI_NETBSD:
            ftrace_write("NetBSD\n");
            break;
        case ELFOSABI_LINUX:
            ftrace_write("Linux\n");
            break;
        case ELFOSABI_SOLARIS:
            ftrace_write("Sun Solaris\n");
            break;
        case ELFOSABI_AIX:
            ftrace_write("IBM AIX\n");
            break;
        case ELFOSABI_IRIX:
            ftrace_write("SGI Irix\n");
            break;
        case ELFOSABI_FREEBSD:
            ftrace_write("FreeBSD\n");
            break;
        case ELFOSABI_TRU64:
            ftrace_write("Compaq TRU64 UNIX\n");
            break;
        case ELFOSABI_MODESTO:
            ftrace_write("Novell Modesto\n");
            break;
        case ELFOSABI_OPENBSD:
            ftrace_write("OpenBSD\n");
            break;
        case ELFOSABI_ARM_AEABI:
            ftrace_write("ARM EABI\n");
            break;
        case ELFOSABI_ARM:
            ftrace_write("ARM\n");
            break;
        case ELFOSABI_STANDALONE:
            ftrace_write("Standalone (embedded) app\n");
            break;
        default:
            ftrace_write("Unknown (0x%x)\n", eh.e_ident[EI_OSABI]);
            break;
    }

    /* ELF filetype */
    ftrace_write("Filetype \t= ");
    switch (eh.e_type) 
    {
        case ET_NONE:
            ftrace_write("N/A (0x0)\n");
            break;
        case ET_REL:
            ftrace_write("Relocatable\n");
            break;
        case ET_EXEC:
            ftrace_write("Executable\n");
            break;
        case ET_DYN:
            ftrace_write("Shared Object\n");
            break;
        default:
            ftrace_write("Unknown (0x%x)\n", eh.e_type);
            break;
    }

    /* ELF Machine-id */
    ftrace_write("Machine\t\t= ");
    switch (eh.e_machine) 
    {
        case EM_NONE:
            ftrace_write("None (0x0)\n");
            break;
        case EM_386:
            ftrace_write("INTEL x86 (0x%x)\n", EM_386);
            break;
        case EM_X86_64:
            ftrace_write("AMD x86_64 (0x%x)\n", EM_X86_64);
            break;
        case EM_AARCH64:
            ftrace_write("AARCH64 (0x%x)\n", EM_AARCH64);
            break;
        default:
            ftrace_write(" 0x%x\n", eh.e_machine);
            break;
    }

    /* Entry point */
    ftrace_write("Entry point\t= 0x%08lx\n", eh.e_entry);

    /* ELF header size in bytes */
    ftrace_write("ELF header size\t= 0x%08x\n", eh.e_ehsize);

    /* Program Header */
    ftrace_write("Program Header\t= 0x%08lx\n", eh.e_phoff);  /* start */
    ftrace_write("\t\t  %d entries\n", eh.e_phnum);         /* num entry */
    ftrace_write("\t\t  %d bytes\n", eh.e_phentsize);       /* size/entry */

    /* Section header starts at */
    ftrace_write("Section Header\t= 0x%08lx\n", eh.e_shoff); /* start */
    ftrace_write("\t\t  %d entries\n", eh.e_shnum);         /* num entry */
    ftrace_write("\t\t  %d bytes\n", eh.e_shentsize);       /* size/entry */
    ftrace_write("\t\t  0x%08x (string table offset)\n", eh.e_shstrndx);

    /* File flags (Machine specific) */
    ftrace_write("File flags \t= 0x%08x\n", eh.e_flags);

    /* ELF file flags are machine specific. */
    int32_t ef = eh.e_flags;
    ftrace_write("\t\t  ");

    if (ef & EF_ARM_RELEXEC)
        ftrace_write(",RELEXEC ");

    if (ef & EF_ARM_HASENTRY)
        ftrace_write(",HASENTRY ");

    if (ef & EF_ARM_INTERWORK)
        ftrace_write(",INTERWORK ");

    if (ef & EF_ARM_APCS_26)
        ftrace_write(",APCS_26 ");

    if (ef & EF_ARM_APCS_FLOAT)
        ftrace_write(",APCS_FLOAT ");

    if (ef & EF_ARM_PIC)
        ftrace_write(",PIC ");

    if (ef & EF_ARM_ALIGN8)
        ftrace_write(",ALIGN8 ");

    if (ef & EF_ARM_NEW_ABI)
        ftrace_write(",NEW_ABI ");

    if (ef & EF_ARM_OLD_ABI)
        ftrace_write(",OLD_ABI ");

    if (ef & EF_ARM_SOFT_FLOAT)
        ftrace_write(",SOFT_FLOAT ");

    if (ef & EF_ARM_VFP_FLOAT)
        ftrace_write(",VFP_FLOAT ");

    if (ef & EF_ARM_MAVERICK_FLOAT)
        ftrace_write(",MAVERICK_FLOAT ");

    ftrace_write("\n");

    /* MSB of flags contains ARM EABI version */
    ftrace_write("ARM EABI\t= Version %d\n", (ef & EF_ARM_EABIMASK) >> 24);

    ftrace_write("\n"); /* End of ELF header */
}

//从文件描述符 fd 中读取 ELF 段头 sh 指定的段数据，并将其存储在 dst 指向的内存区域
static void read_section(int fd, Elf64_Shdr sh, void *dst) 
{
    assert(dst != NULL);
    
    off_t offset = (off_t)sh.sh_offset;
    ssize_t size = sh.sh_size;

    if (lseek(fd, offset, SEEK_SET) != offset) 
        panic("Failed to seek to section offset");

    if (read(fd, dst, size) != size) 
        panic("Failed to read section data");
}

//函数从文件描述符 fd 中读取 ELF 文件的段头信息，并存储在 sh_tbl 指向的内存区域
static void read_section_headers(int fd, Elf64_Ehdr eh, Elf64_Shdr *sh_tbl) 
{
    off_t offset = eh.e_shoff;
    ssize_t size = eh.e_shentsize;

    // 将文件指针定位到段头表的起始位置
    if (lseek(fd, offset, SEEK_SET) != offset) 
        panic("Failed to seek to section header table offset");

    // 逐个读取段头信息
    for (int i = 0; i < eh.e_shnum; i++) 
        if (read(fd, (void *)&sh_tbl[i], size) != size) 
            panic("Failed to read section header");
}

//展示 ELF 文件中的段头信息
static void display_section_headers(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[]) 
{
    // 动态分配内存以读取段头字符串表
    char *sh_str = malloc(sh_tbl[eh.e_shstrndx].sh_size);
    if (sh_str == NULL) 
        panic("Failed to allocate memory for section header string table");
    
    // 读取段头字符串表
    read_section(fd, sh_tbl[eh.e_shstrndx], sh_str);

    // 输出段头信息
    ftrace_write("========================================");
    ftrace_write("========================================\n");
    ftrace_write(" idx offset     load-addr  size       algn"
                 " flags      type       section\n");
    ftrace_write("========================================");
    ftrace_write("========================================\n");

    for (int i = 0; i < eh.e_shnum; i++) 
    {
        ftrace_write(" %03d ", i);
        ftrace_write("0x%08lx ", sh_tbl[i].sh_offset);
        ftrace_write("0x%08lx ", sh_tbl[i].sh_addr);
        ftrace_write("0x%08lx ", sh_tbl[i].sh_size);
        ftrace_write("%-4ld ", sh_tbl[i].sh_addralign);
        ftrace_write("0x%08lx ", sh_tbl[i].sh_flags);
        ftrace_write("0x%08x ", sh_tbl[i].sh_type);
        ftrace_write("%s\t", (sh_str + sh_tbl[i].sh_name));
        ftrace_write("\n");
    }
    
    ftrace_write("========================================");
    ftrace_write("========================================\n");
    ftrace_write("\n"); /* end of section header table */

    // 释放段头字符串表的内存
    free(sh_str);
}

//从 ELF 文件中读取符号表和字符串表，并输出符号表信息。同时，符号表信息存储在全局变量 symbol_tbl 中
static void read_symbol_table(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[], int sym_idx) 
{
    // 动态分配内存以读取符号表
    Elf64_Sym *sym_tbl = malloc(sh_tbl[sym_idx].sh_size);
    if (sym_tbl == NULL) 
        panic("Failed to allocate memory for symbol table");
    
    read_section(fd, sh_tbl[sym_idx], sym_tbl);

    // 确定字符串表的索引并动态分配内存
    int str_idx = sh_tbl[sym_idx].sh_link;
    char *str_tbl = malloc(sh_tbl[str_idx].sh_size);
    if (str_tbl == NULL) 
        panic("Failed to allocate memory for string table");
    
    read_section(fd, sh_tbl[str_idx], str_tbl);

    int sym_count = sh_tbl[sym_idx].sh_size / sizeof(Elf64_Sym);
    // 输出符号表信息
    ftrace_write("Symbol count: %d\n", sym_count);
    ftrace_write("====================================================\n");
    ftrace_write(" num    value            type size       name\n");
    ftrace_write("====================================================\n");
    for (int i = 0; i < sym_count; i++) 
    {
        ftrace_write(" %-3d    %016lx %-4d %-10ld %s\n",
            i,
            sym_tbl[i].st_value, 
            ELF64_ST_TYPE(sym_tbl[i].st_info),
            sym_tbl[i].st_size,
            str_tbl + sym_tbl[i].st_name
        );
    }
    ftrace_write("====================================================\n\n");

    // 将符号表信息存储在全局变量 symbol_tbl 中
    symbol_tbl_size = sym_count;
    symbol_tbl = malloc(sizeof(SymEntry) * sym_count);
    if (symbol_tbl == NULL) 
        panic("Failed to allocate memory for symbol table entries");
    
    for (int i = 0; i < sym_count; i++) 
    {
        symbol_tbl[i].addr = sym_tbl[i].st_value;
        symbol_tbl[i].info = sym_tbl[i].st_info;
        symbol_tbl[i].size = sym_tbl[i].st_size;
        memset(symbol_tbl[i].name, 0, 32);
        strncpy(symbol_tbl[i].name, str_tbl + sym_tbl[i].st_name, 31);
    }

    // 释放动态分配的内存
    free(sym_tbl);
    free(str_tbl);
}

//遍历 ELF 文件的段头表，并根据段类型调用 read_symbol_table 函数读取符号表
static void read_symbols(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[]) 
{
    for (int i = 0; i < eh.e_shnum; i++) 
        if (sh_tbl[i].sh_type == SHT_SYMTAB || sh_tbl[i].sh_type == SHT_DYNSYM) 
            read_symbol_table(fd, eh, sh_tbl, i);
}

//用于初始化一个尾递归调用链表
static void init_tail_rec_list() 
{
    tail_rec_head = (TailRecNode *)malloc(sizeof(TailRecNode));
    if (tail_rec_head == NULL) 
        panic("Failed to allocate memory for tail recursion list");

    tail_rec_head->pc = 0;
    tail_rec_head->depend = 0;
    tail_rec_head->next = NULL;
}

//从 ELF 文件中读取并解析文件头、段头、符号表等信息，并初始化一个尾递归调用链表
void parse_elf(const char *elf_file) 
{
    if (elf_file == NULL) return;

    Log("specified ELF file: %s\n", elf_file);
    int fd = open(elf_file, O_RDONLY | O_SYNC);
    Assert(fd >= 0, "Error %d: unable to open %s\n", fd, elf_file);

    Elf64_Ehdr eh;
    read_elf_header(fd, &eh);
    display_elf_header(eh);

    Elf64_Shdr *sh_tbl = malloc(eh.e_shnum * sizeof(Elf64_Shdr));
    if (sh_tbl == NULL) 
        panic("Failed to allocate memory for section headers");

    read_section_headers(fd, eh, sh_tbl);
    display_section_headers(fd, eh, sh_tbl);

    read_symbols(fd, eh, sh_tbl);

    init_tail_rec_list();

    free(sh_tbl);  // 释放段头表内存
    close(fd);
}

// 用于在符号表中查找目标地址 target 所对应的函数符号
static int find_symbol_func(paddr_t target, bool is_call) 
{
    for (int i = 0; i < symbol_tbl_size; i++) 
    {
        if (ELF64_ST_TYPE(symbol_tbl[i].info) == STT_FUNC) 
        {
            if (is_call) 
            {
                // 查找函数调用的精确地址匹配
                if (symbol_tbl[i].addr == target) 
                {
                    return i;
                }
            } 
            else 
            {
                // 查找函数返回地址在函数范围内
                if (symbol_tbl[i].addr <= target && target < symbol_tbl[i].addr + symbol_tbl[i].size) 
                {
                    return i;
                }
            }
        }
    }
    return -1; // 未找到匹配的符号
}

// 用于在尾递归调用链表中插入一个新节点
static void insert_tail_rec(paddr_t pc, paddr_t depend) 
{
    if (tail_rec_head == NULL) 
        panic("Tail recursion list is not initialized");

    TailRecNode *node = (TailRecNode *)malloc(sizeof(TailRecNode));
    if (node == NULL) 
        panic("Failed to allocate memory for new tail recursion node");

    node->pc = pc;
    node->depend = depend;
    node->next = tail_rec_head->next;
    tail_rec_head->next = node;
}

// 用于从尾递归调用链表中移除头节点后面的第一个节点
static void remove_tail_rec() 
{
    if (tail_rec_head == NULL || tail_rec_head->next == NULL) 
        panic("Tail recursion list is empty or not initialized");

    TailRecNode *node = tail_rec_head->next;
    tail_rec_head->next = node->next;
    free(node);
}

//记录函数调用
void trace_func_call(paddr_t pc , paddr_t target , bool is_tail)
{
  if(symbol_tbl == NULL)
    return ; 

  call_depth++ ;

// 忽略 _trm_init 和 main 函数
  if(call_depth <= 2) 
  {
    return ; 
  }

    int i = find_symbol_func(target, true);  // 查找目标地址对应的符号
    ftrace_write(FMT_PADDR ": %*scall [%s@" FMT_PADDR "]\n",
		pc,                                     // 当前程序计数器
		(call_depth - 3) * 2, "",                   // 缩进
		i >= 0 ? symbol_tbl[i].name: "???",          // 符号名称或默认值
		target                                  // 目标地址
	);

  if (is_tail) 
		insert_tail_rec(pc, target);
}

// 记录函数返回
void trace_func_ret(paddr_t pc) 
{
	if (symbol_tbl == NULL) 
    {
        return;
    }

	if (call_depth <= 2) 
    {
        return; 
    }

	int i = find_symbol_func(pc, false);
	ftrace_write(FMT_PADDR ": %*sret [%s]\n", // 查找当前地址对应的符号
		pc,                                     // 当前程序计数器
		(call_depth-3)*2, "",                   // 缩进
		i>=0?symbol_tbl[i].name:"???"           // 符号名称或默认值
	);
	
	call_depth--;

  TailRecNode *node = tail_rec_head->next;
	if (node != NULL) 
  {
		int depend_i = find_symbol_func(node->depend, true);
		if (depend_i == i) 
    {
			paddr_t ret_target = node->pc;
			remove_tail_rec();
			trace_func_ret(ret_target);
		}
	}
}
