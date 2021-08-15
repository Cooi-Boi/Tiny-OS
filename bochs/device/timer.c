#include "timer.h"
#include "io.h"
#include "print.h"
#include "../kernel/interrupt.h"
#include "../thread/thread.h"
#include "debug.h"
#include "global.h"

#define IRQ0_FREQUENCY 	100
#define INPUT_FREQUENCY        1193180
#define COUNTER0_VALUE		INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT		0X40
#define COUNTER0_NO 		0
#define COUNTER_MODE		2
#define READ_WRITE_LATCH	3
#define PIT_COUNTROL_PORT	0x43
#define mil_second_per_init	1000 / IRQ0_FREQUENCY

//自中断开启以来总的滴答数
uint32_t ticks;

void frequency_set(uint8_t counter_port ,uint8_t counter_no,uint8_t rwl,uint8_t counter_mode,uint16_t counter_value)
{
    outb(PIT_COUNTROL_PORT,(uint8_t) (counter_no << 6 | rwl << 4 | counter_mode << 1));
    outb(counter_port,(uint8_t)counter_value);
    outb(counter_port,(uint8_t)counter_value >> 8);
    return;
} 

void intr_timer_handler(void)
{
    struct task_struct* cur_thread = running_thread();   //得到pcb指针
    ASSERT(cur_thread->stack_magic == 0x23333333);	       //检测栈是否溢出
    
    ++ticks;
    ++cur_thread->elapsed_ticks;
    if(!cur_thread->ticks)
    {
    	schedule();
    }
    else    
    	--cur_thread->ticks;
    return;
}

void timer_init(void)
{
    put_str("timer_init start!\n");
    frequency_set(COUNTER0_PORT,COUNTER0_NO,READ_WRITE_LATCH,COUNTER_MODE,COUNTER0_VALUE);
    register_handler(0x20,intr_timer_handler);	        //注册时间中断函数 0x20向量号函数更换
    put_str("timer_init done!\n");
    return;
}

//休息n个时间中断期
void ticks_to_sleep(uint32_t sleep_ticks)
{
    uint32_t start_tick = ticks;
    while(ticks - start_tick < sleep_ticks)
    	thread_yield();
}

//毫秒为单位 通过毫秒的中断数来调用ticks_to_sleep 来达到休息毫秒的作用
void mtime_sleep(uint32_t m_seconds)
{
    uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds,mil_second_per_init);
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}
