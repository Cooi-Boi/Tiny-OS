#ifndef __USERPROG__PROCESS_H
#define __USERPROG__PROCESS_H

#define USER_VADDR_START 0x8048000

extern void intr_exit(void);

void start_process(void* filename_);
void page_dir_activate(struct task_struct* p_thread);
void process_activate(struct task_struct* p_thread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);
void process_execute(void* filename,char* name);

#endif
