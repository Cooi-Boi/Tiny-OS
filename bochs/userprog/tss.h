#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H
#include "../thread/thread.h"

//内核栈都是位于 pcb头顶一页的地方
void updata_tss_esp(struct task_struct* pthread);
struct gdt_desc make_gdt_desc(uint32_t* desc_addr,uint32_t limit,uint8_t attr_low,uint8_t attr_high);
void tss_init(void);

#endif
