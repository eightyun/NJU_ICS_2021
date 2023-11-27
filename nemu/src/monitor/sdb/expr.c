#include <isa.h>
#include "/home/sakura/NJU_PA_ics2021/nemu/include/memory/vaddr.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256,

  /* TODO: Add more token types */
  TK_POS , TK_DEREF , TK_NEG , // POS 正号运算符    NEG 负号运算符   DEREF 指针解引用
  TK_EQ , TK_NE , TK_GT , TK_LT , TK_GE , TK_LE ,   
  TK_AND , TK_OR ,  
  TK_REG ,   //  register
  TK_NUM ,  // 10 & 16 
};

#define OFTYPES(type , types) oftypes(type , types , ARRLEN(types)) 

static int bound_types[] = {')' , TK_NUM , TK_REG} ; // boundary for binary operator
__attribute__((unused)) static int not_types[] = {'(' , ')' , TK_NUM , TK_REG} ;  // not operator type 
static int op_types[] = {TK_NEG , TK_POS , TK_DEREF} ; // unary operator type

static bool oftypes(int type , int types[] , int size)
{
  for(int i = 0 ; i < size ; i++)
    if(type == types[i])
      return true ;
  
  return false ;
}

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\(" , '('} ,       // left bracket
  {"\\)" , ')'} ,       // lighr bracket
  {"\\+", '+'},         // plus
  {"-" , '-'},        // subtract
  {"\\*" , '*'} ,       // multiply
  {"/" ,  '/'} ,      // divide
  {"<" , TK_LT},       // less than
  {">" , TK_GT},       // greater than
  {"<=" , TK_LE},      // less equal
  {">=" , TK_GE},      // greater equal
  {"==", TK_EQ},        // equal
  {"!=" , TK_NE},       // not equal 
  {"&&" , TK_AND},      // and
  {"\\|\\|" , TK_OR},   // or
  {"(0x)?[0-9]+" , TK_NUM},  // decimalism and hexadecimal
  {"\\$\\w+" , TK_REG} ,  // register
  {"[A-Za-z_]\\w*" , TK_REG} , 
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() { // 被编译成一些用于进行pattern匹配的内部信息
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {}; // 用于按顺序存放已经被识别出的token信息
static int nr_token __attribute__((used))  = 0; // 指示已经被识别出的token数目

static bool make_token(char *e) { // 识别出其中的token
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') { // position变量来指示当前处理到的位置 并且按顺序尝试用不同的规则来匹配当前位置的字符串
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            //i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        
        if(rules[i].token_type == TK_NOTYPE) break ;

        tokens[nr_token].type = rules[i].token_type ;

        switch (rules[i].token_type) 
        {
          case TK_NUM: case TK_REG:
          
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          break ;

          case '*' : case '-' : case '+' :

          if(nr_token == 0 || !OFTYPES(tokens[nr_token - 1].type , bound_types))
          {
            switch(rules[i].token_type)
            {
              case '*' : tokens[nr_token].type = TK_DEREF ; break ;
              case '-' : tokens[nr_token].type = TK_NEG ; break ;
              case '+' : tokens[nr_token].type = TK_POS ; break ;
            }
          }
        }
        nr_token ++;

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  
  return true;
}

// unary operator
static word_t cal1(int op , word_t val , bool * success)
{
  switch(op)
  {
    case TK_POS :  return val ;
    case TK_NEG :  return -val ;
    case TK_DEREF :  return vaddr_read(val , 8) ;

    default : *success = false ;
  }

  return 0 ;
}

// binary operator
static word_t cal2(word_t val1 , int op , word_t val2 , bool * success)
{
  switch(op)
  {
    case '+' : return val1 + val2 ;
    case '-' : return val1 - val2 ;
    case '*' : return val1 * val2 ;
    case '/' : 
      if(val2 == 0)
      {
        *success = false ;
        return 0 ;
      }
      return (sword_t)val1 / (sword_t)val2 ;
    case TK_AND : return val1 && val2 ;
    case TK_OR  : return val1 || val2 ;
    case TK_EQ :  return val1 == val2 ;
    case TK_NE :  return val1 != val2 ;
    case TK_LT :  return val1 < val2 ;
    case TK_LE :  return val1 <= val2 ;
    case TK_GT :  return val1 > val2 ;
    case TK_GE :  return val1 >= val2 ;

    default : *success = false ;
  }

  return 0 ;
}

static word_t eval_operand(int i , bool * success)
{
  switch(tokens[i].type)
  {
    case TK_NUM:
      if(strncmp(tokens[i].str, "0x", 2) == 0) return strtol(tokens[i].str , NULL , 16) ;
      else return strtol(tokens[i].str , NULL , 10) ;

    case TK_REG:
      return isa_reg_str2val(tokens[i].str , success) ;

    default:
      *success = false ;
      return 0 ;
  }
}

static bool check_parentheses(int p, int q)
{
  if (tokens[p].type == '(' && tokens[q].type == ')')
  {
    int par = 0 ; // 未匹配的左括号
    
    for(int i = p ; i <= q ; i++)
    {
      if(tokens[i].type == '(') par ++ ;
      else if(tokens[i].type == ')') par -- ;

      if(par == 0) return i == q ;  // the leftest parenthese is matched
    }  
  }

  return false ;
}

static int find_major(int p , int q) // 找到主要符号
{
  int ret = -1 , par = 0 , op_major = 0 ;

  for(int i = p ; i <= q ; i++)
  {
    if(tokens[i].type == TK_NUM)
      continue ; 

    if(tokens[i].type == '(')
      par++ ;

    else if(tokens[i].type == ')')
    {
      if(par == 0)
        return -1 ;

      par-- ;  
    }
    else if(par > 0)
      continue ;
    else 
    {
      int tmp_major = 0 ;
      switch(tokens[i].type)
      {
        case TK_OR : tmp_major ++ ; 
        case TK_AND : tmp_major ++ ;
        case TK_EQ : case TK_NE : tmp_major ++ ;
        case TK_LT : case TK_LE : case TK_GT : case TK_GE : tmp_major ++ ;
        case '+' : case '-' : tmp_major ++ ;
        case '*' : case '/' : tmp_major ++ ;
        case TK_POS : case TK_DEREF : case TK_NEG : tmp_major ++ ; break ;
        default : return -1 ;
      }

      if(tmp_major > op_major  || (tmp_major == op_major && !OFTYPES(tokens[i].type , op_types))) 
      {
        op_major = tmp_major ;
        ret = i ;
      } 
    }  
  }

  if(par != 0) return -1 ;

  return ret ;
}

static word_t eval(int p , int q , bool * success) 
{
  *success = true ;

  if (p > q)
  {
    *success = false ;
    return 0 ;
  }
  else if (p == q) 
  {
    return eval_operand(p , success) ;
  }
  else if (check_parentheses(p , q)) 
  {
    return eval(p + 1 , q - 1 , success);
  }
  else 
  {
    int major = find_major(p , q);
    if(major < 0)
    {
      *success = false ;
      return 0 ;
    }

    bool success_left , success_right ;
    word_t val1 = eval(p , major - 1 , &success_left);    // 递归左边
    word_t val2 = eval(major + 1 , q , &success_right);  // 递归右边

    if(!success_right)
    {
      *success = false ;
      return 0 ;
    }

    if(success_left)
    {
      word_t ret = cal2(val1 , tokens[major].type , val2 , success) ;
      return ret ;
    }
    else 
    {
      word_t ret = cal1(tokens[major].type , val2 , success) ;
      return ret ;
    }
  }

  return 0 ;
}


word_t expr(char *e, bool *success) 
{
  if (!make_token(e)) 
  {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */


  return eval(0 , nr_token - 1 , success);
}



