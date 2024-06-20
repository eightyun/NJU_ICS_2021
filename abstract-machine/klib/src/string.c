#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) 
{
  const char * p = s ;
  while(*p)
    p++ ;

  return p - s ;
}

//strcpy 函数会将源字符串复制到目标字符串，直到遇到终止符 '\0'
//它不会检查目标字符串的长度，因此目标字符串必须足够大以容纳源字符串，否则可能导致缓冲区溢出。
char *strcpy(char *dst, const char *src) 
{
  // 确保源字符串不为 NULL
  if(src == NULL)
    return NULL ;

  char * original_dst = dst ;
  // 逐字符复制源字符串到目标字符串，直到遇到终止符 '\0'
  while((*dst++ = *src++)) ;
  // 返回目标字符串的起始地址
  return original_dst ;
}

//strncpy 函数会将最多 n 个字符从源字符串复制到目标字符串
//如果源字符串长度小于 n，则目标字符串中多余的部分会用 '\0' 填充
//如果源字符串长度大于或等于 n，目标字符串不会自动添加终止符 '\0'，因此需要手动处理
char *strncpy(char *dst, const char *src, size_t n) 
{
  char * original_dst = dst ;
  size_t i ;

  for(i = 0 ; i < n && src[i] != '\0' ; i++)
    dst[i] = src[i] ;

  for( ; i < n ; i++)
    dst[i] = '\0' ;

  return original_dst ;
}

//strcat 函数用于将一个字符串追加到另一个字符串的末尾
char *strcat(char *dst, const char *src) 
{
  char * original_dst = dst ;

  // 移动到目标字符串的末尾
  while(*dst)
    dst++ ;

  // 逐字符复制源字符串到目标字符串
  while((*dst++ = *src++)) ;

  return original_dst ;
}

//strcmp 函数用于比较两个字符串的内容。它会逐字符地比较字符串，直到发现不同的字符或遇到字符串的终止符 '\0'。
int strcmp(const char *s1, const char *s2) 
{
  while(*s1 && (*s1 == *s2))
  {
    ++s1 ; 
    ++s2 ;
  }

  return *(unsigned char *)s1 - *(unsigned char *)s2 ;
}

//strncmp 函数用于比较两个字符串的前 n 个字符
int strncmp(const char *s1, const char *s2, size_t n) 
{
    while(n > 0 && *s1 && (*s1 == *s2))
    {
      ++s1 ;
      ++s2 ;
      n-- ;
    }

    if(n == 0)
      return 0 ;

    return *(unsigned char *)s1 - *(unsigned char *)s2 ;
}

//memset 函数用于将指定值填充到一块内存区域
void *memset(void *s, int c, size_t n) 
{
  unsigned char * p = s ;
  while(n--)
    *p++ = (unsigned char) c ;

  return s ;
}

//memmove 函数用于在内存中复制一块数据到另一块重叠的内存区域
void *memmove(void *dst, const void *src, size_t n) 
{
  unsigned char * d = dst ;
  const unsigned char * s = src ;

  // 如果目标地址在源地址之前，直接从前向后复制
  if(d < s)
  {
    while(n--)
      *d++ = *s++ ;
  }
  // 如果目标地址在源地址之后，从后向前复制
  else
  {
    d += n ;
    s += n ;
    while(n--)
      *--d = *--s ;
  }
    
  return dst ;
}

//memcpy 函数用于从源地址复制一块内存内容到目标地址 与 memmove 不同，memcpy 假定源和目标内存区域不重叠
void *memcpy(void *out, const void *in, size_t n) 
{
  unsigned char *d = out;
  const unsigned char *s = in;

    // 逐字节复制源数据到目标区域
  while (n--)
      *d++ = *s++;

  return d;
}

//memcmp 函数用于比较内存块中的数据。它逐字节比较两个内存块的内容，直到找到不同的字节或比较完指定的字节数。
int memcmp(const void *s1, const void *s2, size_t n) 
{
  const unsigned char * p1 = s1 ;
  const unsigned char * p2 = s2 ;

  while(n--)
  {
    if(*p1 != *p2)
      return (int)(*p1 - *p2) ;

    p1++ ;
    p2++ ;
  }

  return 0 ; 
}

#endif
