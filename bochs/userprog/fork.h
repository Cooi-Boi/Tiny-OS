#ifndef __USERPROG__FORK_H
#define __USERPROG__FORK_H

#include "stdint.h"
#include "string.h"
#include "../thread/thread.h"

int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread,struct task_struct* parent_thread);
void copy_body_stack3(struct task_struct* parent_thread,struct task_struct* child_thread,void* buf_page);
int32_t build_child_stack(struct task_struct* child_thread);
void updata_inode_open_cnts(struct task_struct* thread);
int32_t copy_process(struct task_struct* child_thread,struct task_struct* parent_thread);
pid_t sys_fork(void);

#endif
