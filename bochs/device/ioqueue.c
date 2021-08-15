#include "ioqueue.h"
#include "interrupt.h"
#include "global.h"
#include "debug.h"


void init_ioqueue(struct ioqueue* ioq)
{
    lock_init(&ioq->lock);
    ioq->consumer = ioq->producer = NULL;
    ioq->head = ioq->tail = 0;
}

uint32_t next_pos(uint32_t pos)
{
    return (pos+1)%bufsize;
}

bool ioq_full(struct ioqueue* ioq)
{
    ASSERT(intr_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}

bool ioq_empty(struct ioqueue* ioq)
{
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}

void ioq_wait(struct task_struct** waiter)
{
    ASSERT(waiter != NULL && *waiter == NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

void wakeup(struct task_struct** waiter)
{
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

char ioq_getchar(struct ioqueue* ioq)
{
    ASSERT(intr_get_status() == INTR_OFF);
    
    //空的时候就需要动锁了 把另外的消费者卡住
    while(ioq_empty(ioq))
    {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }
    
    char retchr = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);
    
    if(ioq->producer)
    	wakeup(&ioq->producer);
    return retchr;
}

void ioq_putchar(struct ioqueue* ioq,char chr)
{
    ASSERT(intr_get_status() == INTR_OFF);
    
    while(ioq_full(ioq))
    {
    	lock_acquire(&ioq->lock);
    	ioq_wait(&ioq->producer);
    	lock_release(&ioq->lock);
    }
    
    ioq->buf[ioq->head] = chr;
    ioq->head = next_pos(ioq->head);
    
    if(ioq->consumer)
    	wakeup(&ioq->consumer);
}
