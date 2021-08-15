#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

void panic_spin(char* filename,int line,const char* func,const char* condition);

#define PANIC(...) panic_spin (__FILE__,__LINE__,__func__,__VA_ARGS__)

#ifdef NDEBUG 
#define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION) \
   if(CONDITION){}        \
   else{ PANIC(#CONDITION); }

#endif

#endif

