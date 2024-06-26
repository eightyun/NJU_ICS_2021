#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// 输出字符串的简单实现
void puts(const char* str) 
{
    while (*str) 
        putch(*str++);
}

// 输出整数的简单实现
void putint(int num) 
{
    if (num == 0) 
    {
        putch('0');
        return;
    }

    if (num < 0) 
    {
        putch('-');
        num = -num;
    }

    char buf[10];
    int i = 0;

    while (num > 0) 
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) 
        putch(buf[--i]);
}

int printf(const char* fmt, ...) 
{
    va_list args;
    va_start(args, fmt);

    int count = 0;
    while (*fmt)
     {
        if (*fmt == '%') 
        {
            fmt++;

            if (*fmt == 'd') 
            {
                int num = va_arg(args, int);
                putint(num);
                count++;
            } 
            else if (*fmt == 's') 
            {
                char* str = va_arg(args, char*);
                puts(str);
                count++;
            } 
            else 
            {
                putch('%');
                putch(*fmt);
            }
        } 
        else 
        {
            putch(*fmt);
        }

        fmt++;
    }

    va_end(args);
    return count;
}

// ----------------------------------------------------------------------------------------------

// 将字符追加到输出缓冲区
static void append_char(char **out, char c) 
{
  //将字符 c 放入缓冲区的当前位置
    **out = c; 
    (*out)++;
}

// 将字符串追加到输出缓冲区
static void append_string(char **out, const char *str) 
{
    while (*str) 
        append_char(out, *str++);
}

//将整数 num 转换为字符串并追加到输出缓冲区 out
static void append_int(char **out, int num) 
{
    char buf[10];
    int i = 0;
    if (num == 0) 
    {
        append_char(out, '0');
        return;
    }

    if (num < 0) 
    {
        append_char(out, '-');
        num = -num;
    }

    while (num > 0) 
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) 
        append_char(out, buf[--i]);
}

//vsprintf 是一个与 printf 类似的函数，用于格式化输出，但它将结果存储在一个字符串缓冲区中，而不是输出到标准输出。
int vsprintf(char *out, const char *fmt, va_list ap) 
{
    char *ptr = out; 
    
    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++; // 移动到格式字符后面
            
            if (*fmt == 'd')
            {
                int num = va_arg(ap, int);
                append_int(&ptr, num);
            }
            else if (*fmt == 's')
            {
                char *str = va_arg(ap, char*);
                append_string(&ptr, str);
            }
            else
            {
                append_char(&ptr, '%'); // 处理 %% 的情况
                append_char(&ptr, *fmt);
            }
        }
        else
        {
            append_char(&ptr, *fmt);
        }

        fmt++;
    }

    *ptr = '\0';
    return ptr - out;
}

// -----------------------------------------------------------------------------------------------

//sprintf用于将格式化的数据写入字符串缓冲区 它的作用与 printf 类似但不同之处在于 
//printf 是将格式化的数据输出到标准输出（通常是屏幕），而 sprintf 是将格式化的数据写入到一个指定的字符数组中。
int sprintf(char *out, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(out, fmt, args);
  va_end(args);
  return ret;
}

//格式化输出到一个字符数组中，并限制最大输出字符数，但它接受一个 va_list 类型的参数列表，而不是可变参数列表（...）
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) 
{
  // 使用 vsnprintf 函数进行格式化输出
    int ret = vsnprintf(out, n, fmt, ap);

    // 返回 vsnprintf 的返回值
    return ret;
}

//用于格式化字符串输出到一个字符数组中，并且能够限制输出的最大字符数，防止缓冲区溢出
int snprintf(char *out, size_t n, const char *fmt, ...) 
{
  va_list args ;
  int ret ; 

  va_start(args , fmt) ; 
  ret = vsnprintf(out , n , fmt , args) ; 
  va_end(args) ; 

  return ret ;
}

#endif
