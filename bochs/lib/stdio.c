#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"

void itoa(uint32_t value,char** buf_ptr_addr,uint8_t base)//这里有理解的 为了修改一级指针的值 再取一次& 即可修改一级指针本身的值
{
    uint32_t m = value % base;
    uint32_t i = value / base;  //除数为0即最高位了 输出即可 没到零继续即可
    if(i)
    	itoa(i,buf_ptr_addr,base);
    if(m < 10)                  //m小于10的数
    	*((*buf_ptr_addr)++) = m + '0';
    else			 //m大于10的数
    	*((*buf_ptr_addr)++) = m + 'A' - 10;
}

uint32_t vsprintf(char* str,const char* format,va_list ap)
{
    char* buf_ptr = str;
    const char* index_ptr = format;
    char index_char = *index_ptr;
    int32_t arg_int;
    char* arg_str;
    while(index_char)		//挨个挨个字符来弄
    {
    	if(index_char != '%')
    	{
    	    *(buf_ptr++) = index_char;
    	    index_char = *(++index_ptr);
    	    continue;
    	}
    	index_char = *(++index_ptr);
    	switch(index_char)
    	{
    	    case 's':
    	    	arg_str = va_arg(ap,char*);
    	    	strcpy(buf_ptr,arg_str);
    	    	buf_ptr += strlen(arg_str);
    	    	index_char = *(++index_ptr);
    	    	break;
    	    case 'x':
    	    	arg_int = va_arg(ap,int);
    	    	itoa(arg_int,&buf_ptr,16);
    	    	index_char = *(++index_ptr);
    	    	break;
    	    case 'd':
    	    	arg_int = va_arg(ap,int);
    	    	if(arg_int < 0)
    	    	{
    	    	    arg_int = 0 - arg_int;
    	    	    *(buf_ptr++) = '-';
    	    	}
    	    	itoa(arg_int,&buf_ptr,10);
    	    	index_char = *(++index_ptr);
    	    	break;
    	    case 'c':
    	    	*(buf_ptr++) = va_arg(ap,char);
    	    	index_char = *(++index_ptr);
    	}
    }
    return strlen(str);
}

uint32_t printf(const char* format, ...)
{
    va_list args;
    va_start(args,format);		//args指向char* 的指针 方便指向下一个栈参数
    char buf[1024] = {0};
    vsprintf(buf,format,args);
    va_end(args);
    return write(1,buf,strlen(buf));
}

uint32_t sprintf(char* _des,const char* format, ...)
{
    va_list args;
    uint32_t retval;
    va_start(args,format);		//args指向char* 的指针 方便指向下一个栈参数
    retval = vsprintf(_des,format,args);
    va_end(args);
    return retval;
}
