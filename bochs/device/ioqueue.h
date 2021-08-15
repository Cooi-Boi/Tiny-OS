#ifndef __DEVICE__IOQUEUE_H
#define __DEVICE__IOQUEUE_H

#include "stdint.h"
#include "../thread/thread.h"
#include "../thread/sync.h"

#define bufsize 64

struct ioqueue
{
    struct lock lock;
    struct task_struct* consumer;
    struct task_struct* producer;
    char buf[bufsize];
    uint32_t head;			//头部读入数据
    uint32_t tail;			//尾部拿数据
};

void init_ioqueue(struct ioqueue* ioq);
uint32_t next_pos(uint32_t pos);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
void ioq_wait(struct task_struct** waiter); //这里是waiter的二级指针 取二级指针的原因是这样可以对指针的地址值进行修改
void wakeup(struct task_struct** waiter); 
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq,char chr);

#endif
