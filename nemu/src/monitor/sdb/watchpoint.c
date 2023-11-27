#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;                         //NO表示监视点的序号
  struct watchpoint *next;        // 指向下一个监视点

  /* TODO: Add more members if necessary */

  word_t val ;
  char expr[32];
} WP;

static WP wp_pool[NR_WP] = {};       // 使用"池"的数据结构来管理监视点结构体
static WP *head = NULL, *free_ = NULL;  // head用于组织使用中的监视点结构, free_用于组织空闲的监视点结构, init_wp_pool()函数会对两个链表进行初始化

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]); //如果数组已经到达末尾，则将next属性赋值为NULL
    wp_pool[i].val = 0x3f3f3f3f;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

// 实现监视点池的管理
static WP* new_wp()  // new_wp()从free_链表中返回一个空闲的监视点结构  调用new_wp()时可能会出现没有空闲监视点结构的情况, 为了简单起见, 此时可以通过assert(0)马上终止程序
{
  if (free_ == NULL) 
  {
    printf("No free watchpoint\n");
    assert(0);
  }

  WP * new_wp = free_ ;
  free_ = free_->next ;
  new_wp->next = head ;
  head = new_wp ;

  return new_wp ;
}

static void free_wp(WP *wp) // free_wp()将wp归还到free_链表中
{
  WP * h = head ;
  if(h == wp)
    head = NULL ;
  else
  {
    while(h && h->next != wp)
      h = h->next ;

    assert(h) ;
    h->next = wp->next ;
  } 

  wp->next = free_ ;
  free_ = wp ;
}

void wp_pool_display()
{
  WP * h = head ;
  if(h == NULL)
  {
    puts("NO watchpoints") ;
    return  ;
  }
  
  printf("%-8s%-8s\n", "Num", "What");
  while (h) 
  {
    printf("%-8d%-8s\n", h->NO, h->expr);
    h = h->next;
  }
}

void wp_watch(char * expr , word_t value)
{
  WP * wp = new_wp() ;
  strncpy(wp->expr, expr, sizeof(wp->expr) - 1);  // 确保不会溢出
  wp->expr[sizeof(wp->expr) - 1] = '\0';  // 手动添加字符串结束符
  wp->val = value ;
  printf("Watchpoint %d: %s\n" , wp->NO , expr) ;
}

void wp_remove(int n)
{
  assert(n < NR_WP) ;
  WP * wp = &wp_pool[n] ;
  free_wp(wp) ;
  printf("Delete watchpoint %d: %s\n", wp->NO, wp->expr);
}

void wp_difftest()
{
  WP * h = head ;

  while(h)
  {
    bool _ ;
    word_t new = expr(h->expr , &_) ;

    if(h->val != new)
    {
      printf("The value of a watchpoints has changed. The program will be suspended.\n") ;
      printf("Watchpoint %d: %s\n"
        "Old value = %lu\n"
        "New value = %lu\n"
        , h->NO, h->expr, h->val, new);

      h->val = new;

      nemu_state.state = NEMU_STOP ;
    }

    h = h->next ;
  }
}
