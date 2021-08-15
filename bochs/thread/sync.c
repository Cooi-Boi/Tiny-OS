#include "sync.h"
#include "list.h"
#include "stdint.h"
#include "thread.h"
#include "debug.h"
#include "interrupt.h"

void sema_init(struct semaphore* psema,uint32_t value)
{
    psema->value = value;
    list_init(&psema->waiters);
}

void lock_init(struct lock* plock)
{
    sema_init(&plock->semaphore,1);  //信号量初始值都设置成1 
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
}

void sema_down(struct semaphore* psema)
{
    //保证原子操作
    enum intr_status old_status = intr_disable();
    while(!psema->value)
    {
    	//访问锁的不应该在锁的等待队列
    	ASSERT(!elem_find(&psema->waiters,&(running_thread()->general_tag)));
    	if(elem_find(&psema->waiters,&(running_thread()->general_tag)))
    	    PANIC("sema_down: seme_down thread already in ready_list\n");
    	list_append(&psema->waiters,&(running_thread()->general_tag));  //添加到等待队列
    	thread_block(TASK_BLOCKED);                                     //阻塞并切换线程
    }
    --psema->value;							  //占用信号量
    ASSERT(!psema->value);	
    intr_set_status(old_status);	
}

//信号量value增加
void sema_up(struct semaphore* psema)
{

    enum intr_status old_status = intr_disable();
    ASSERT(!psema->value);
    if(!list_empty(&psema->waiters))
    {
    	thread_unblock((struct task_struct*)((uint32_t)list_pop(&psema->waiters) & 0xfffff000));
    }
    ++psema->value;
    ASSERT(psema->value == 1);
    intr_set_status(old_status);
}

//获取锁资源
void lock_acquire(struct lock* plock)
{
    if(plock->holder != running_thread())
    {
    	sema_down(&plock->semaphore);		//如果已经被占用 则会被阻塞
    	plock->holder = running_thread();	//之前被阻塞的线程 继续执行 没被阻塞直接继续即可
    	ASSERT(!plock->holder_repeat_nr);
    	plock->holder_repeat_nr = 1;			//访问数为1
    }
    else	++plock->holder_repeat_nr;
}

//释放锁资源
void lock_release(struct lock* plock)
{
    ASSERT(plock->holder == running_thread());	//释放锁的线程必须是其拥有者
    if(plock->holder_repeat_nr > 1)	//减少到的当前一个线程 次数只有一次访问这个锁时 才允许到下面
    {
    	--plock->holder_repeat_nr;		
    	return;
    }
    ASSERT(plock->holder_repeat_nr == 1);	//举个例子 该线程在拥有锁时 两次获取锁 第二次肯定是无法获取到的
    						//但是必须同样也要有两次release才算彻底释放 不然只有第一次的relase
    						//第二次都不需要release 就直接释放了 肯定是不行的
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);   
}

