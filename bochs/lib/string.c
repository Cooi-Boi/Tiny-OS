#include "string.h"
#include "debug.h"
#include "global.h"

void memset(void* dst_,uint8_t value,uint32_t size)
{
    ASSERT(dst_ != NULL);
    uint8_t* dst = (uint8_t*) dst_;
    while((size--) > 0)
    	*(dst++) = value;
    return;
}

void memcpy(void* dst_,const void* src_,uint32_t size)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t* dst = dst_;
    const uint8_t* src = src_;
    while((size--) > 0)
    	*(dst++) = *(src++);
    return;
}

int memcmp(const void* a_,const void* b_, uint32_t size)
{
    const char* a = a_;
    const char* b = b_;
    ASSERT(a != NULL || b != NULL);
    while((size--) > 0)
    {
    	if(*a != *b)	return (*a > *b) ? 1 : -1;
   	++a,++b;
    }
    return 0;
}

char* strcpy(char* dsc_,const char* src_)
{
    ASSERT(dsc_ != NULL && src_ != NULL);
    char* dsc = dsc_;
    while((*(dsc_++) = *(src_++) ));
    return dsc;     
}

uint32_t strlen(const char* str)
{
    ASSERT(str != NULL);
    const char* ptr = str;
    while(*(ptr++));
    return (ptr - str - 1);             //例如一个字 1 '\0' ptr会指向'\0'后面一位
}

int8_t strcmp(const char* a,const char* b)
{
    ASSERT(a != NULL && b != NULL);
    while(*a && *a == *b)
    {
    	a++,b++;
    }   
    return (*a < *b) ? -1 : (*a > *b) ; //这个表达式太猛了 用活了
}

char* strchr(const char* str,const char ch)
{
    ASSERT(str != NULL);
    while(*str != 0)
    {
    	if(*str == ch)	return (char*)str;
    	++str;
    } 
    return NULL;
}

char* strrchr(const char* str,const uint8_t ch)
{
    ASSERT(str != NULL);
    char* last_chrptr = NULL;
    while(*str != 0)
    {
    	if(ch == *str)	last_chrptr = (char*)str;
    	str++;
    }
    return last_chrptr;   
}

char* strcat(char* dsc_,const char* src_)
{
    ASSERT(dsc_ != NULL && src_ != NULL);
    char* str = dsc_;
    while(*(str++));
    str--;
    while(*(str++) = *(src_++));
    return dsc_;
}

char* strchrs(const char* str,uint8_t ch)
{
    ASSERT(str != NULL);
    uint32_t ch_cnt = 0;
    while(*str)
    {
    	if(*str == ch) ++ch_cnt;
    	++str;
    }
    return ch_cnt;
}
