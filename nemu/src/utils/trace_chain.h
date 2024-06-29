#include <elf.h> 
#include "device/map.h" 

//------------------------------------iringbuf----------------------------------------------
void trace_inst(word_t pc, uint32_t inst)  ;
void display_inst();

//------------------------------------mtrace----------------------------------------------

void display_pread(paddr_t addr,int len) ;
void display_pwrite(paddr_t addr, int len, word_t data) ;

//------------------------------------ftrace----------------------------------------------

void read_elf_header(int fd, Elf64_Ehdr *eh) ;  
void display_elf_header(Elf64_Ehdr eh) ;       
void read_section(int fd, Elf64_Shdr sh, void *dst) ;
void read_section_headers(int fd, Elf64_Ehdr eh, Elf64_Shdr *sh_tbl) ;
void display_section_headers(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[])  ;
void read_symbol_table(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[], int sym_idx)  ;
void read_symbols(int fd, Elf64_Ehdr eh, Elf64_Shdr sh_tbl[]) ;
void init_tail_rec_list() ;
void parse_elf(const char *elf_file) ;
int  find_symbol_func(paddr_t target, bool is_call) ;
void insert_tail_rec(paddr_t pc, paddr_t depend) ;
void remove_tail_rec() ;
void trace_func_call(paddr_t pc , paddr_t target , bool is_tail) ;
void trace_func_ret(paddr_t pc) ;

//------------------------------------dtrace----------------------------------------------
void trace_dread(paddr_t addr, int len, IOMap *map) 
void trace_dwrite(paddr_t addr, int len, word_t data, IOMap *map) 