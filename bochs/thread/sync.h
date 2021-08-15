#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "thread.h"

struct semaphore
{
    uint8_t value;	    //信号量的值
    struct list waiters;   //等待队列
};

struct lock
{
    struct task_struct* holder;
    struct semaphore semaphore;
    uint32_t holder_repeat_nr;	//在释放的时候 通过这个值决定需要 释放锁几次
};

//揣测了一下 sema 前面加了个p 应该是ptr 指针的意思
void sema_init(struct semaphore* psema,uint32_t value);
void lock_init(struct lock* plock);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);
#endif
