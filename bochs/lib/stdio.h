#ifndef __LIB__STDIO_H
#define __LIB__STDIO_H
#include "stdint.h"

#define va_start(ap,v) ap = (va_list)&v          //这里把第一个char*地址赋给ap 强制转换一下
#define va_arg(ap,t)   *((t*)(ap +=4))	   //强制类型转换 得到栈中参数
#define va_end(ap)	ap = NULL   

typedef void* va_list;
void itoa(uint32_t value,char** buf_ptr_addr,uint8_t base);
uint32_t vsprintf(char* str,const char* format,va_list ap);
uint32_t sprintf(char* _des,const char* format, ...);
uint32_t printf(const char* format, ...);

#endif
