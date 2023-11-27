#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
word_t vaddr_read(vaddr_t addr, int len) ;
void wp_remove(int n);
void wp_watch(char * expr , word_t value);
void wp_pool_display() ;


/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char * args)
{
  char * arg = strtok(NULL, " ");

  if (arg == NULL) cpu_exec(1);
  else cpu_exec(strtol(arg, NULL, 10)) ;

  return 0;
}

static int cmd_info(char * args)
{
  char * arg = strtok(NULL, " ");
  
  if(arg == NULL) 
  {
    printf("Please enter help\n") ;
    return 0 ;
  }
  else if (strcmp(arg, "r") == 0) 
  {
    isa_reg_display() ;
  }
  else if(strcmp(arg , "w") == 0) 
  {
    wp_pool_display() ;
  }
  else 
  {
    printf("Please enter help\n") ;
  }
  
  return 0 ;
}

static int cmd_x(char * args)
{
  char * arg1 = strtok(NULL, " ");
  if(arg1 == NULL) 
  {
    printf("Please enter help\n") ;
    return 0 ;
  }
  else
  {
    char * arg2 = strtok(NULL, " ");
    if(arg2 == NULL) 
    {
      printf("Please enter help\n") ;
      return 0 ;
    }
    else
    {
      int n = strtol(arg1 , NULL, 10);
      bool success ;
      vaddr_t start_addr = expr(arg2, &success);

      if(!success)
      {
        puts("invalid start addr") ;
      }
      else
      {
        printf("Origin memory address: %#016lx\n",start_addr);

        for(int i = 0 ; i < n ; i++)
        {
          word_t data = vaddr_read(start_addr , 8) ;
          start_addr += 8 ;
          printf("%#016lx\n", data);
        }
      }
    }
    
  }
  return 0 ;
}

static int cmd_p(char * args)
{
  bool success;
  word_t res = expr(args, &success);

  if (!success) 
  {
    puts("invalid expression");
  } 
  else 
  {
    printf("%lu\n", res);
  }
  return 0;
}

static int cmd_w(char * args)
{
  char * arg = strtok(NULL , " ") ;

  if(arg == NULL)
  {
    printf("please entry help") ;
    return 0 ;
  }
  else 
  {
    bool success ;
    word_t val = expr(arg, &success);
    if(!success)
    {
      puts("invalid expression") ;
    }
    else 
    {
      wp_watch(arg, val) ;
    }
  }

  return 0 ;
}

static int cmd_d(char * args)
{
  char * arg = strtok(NULL, " ");

  if(arg == NULL)
  {
    printf("please entry help") ;
    return 0 ;
  }

  int n = strtol(arg , NULL, 10);

  wp_remove(n) ;

  return 0 ;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"si" , "Let the program step N instructions and then pause execution. If N is not given, the default is 1", cmd_si},
  {"info" , "info r: Print register status , info w: Print watch information", cmd_info},
  {"x" , "Evaluate the expression EXPR and use the result as the starting memory Address, output N consecutive 4 bytes in hexadecimal form , e.g. x 10 $esp" , cmd_x},
  {"p" , "calculate the value of the expression EXPR , e.g. p $eax + 1" , cmd_p} ,
  {"w" , "Set a watch , When the value of the expression EXPR changes, program execution is suspended , e.g. w *0x2000" , cmd_w} ,
  {"d" , "Delete a watch with serial number N" , cmd_d} , 
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) {
          if (strcmp(cmd, "q") == 0) { 
            nemu_state.state = NEMU_QUIT; // set "QUIT" state when q
          }  // 报错是由于is_exit_status_bad函数返回了-1，main函数直接返回了此函数返回的结果，make检测到该可执行文件返回了-1，因此报错
          return ;
        }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void test_expr()
{
  FILE * fp = fopen("/home/sakura/NJU_PA_ics2021/nemu/tools/gen-expr/build/input" , "r") ;
  if(fp == NULL) perror("test_expr error") ;

  char * e = NULL ;
  word_t correct_res ;
  size_t len = 0 ; 
  ssize_t read ;
  bool success = false ;

  while(true)
  {
    if(fscanf(fp, "%lu ", &correct_res) == -1) break;
    
    read = getline(&e, &len, fp);
    e[read - 1] = '\0';

    word_t res = expr(e , &success) ;

    assert(success) ;

    if(res != correct_res)
    {
      puts(e) ;
      printf("expected: %lu , got: %lu\n" , correct_res , res) ;
      assert(0) ;
    }

    fclose(fp) ;
    if(e) free(e) ;

    Log("expr test pass") ;
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* test math expression calculation */
  //test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
