#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "global.h"
#include "list.h"
#include "../kernel/memory.h"

#define PG_SIZE 4096
#define MAX_FILES_OPEN_PER_PROC 8 
typedef int16_t pid_t;
extern struct list thread_ready_list,thread_all_list;

typedef void thread_func(void*); //这里有点不懂定义的什么意思 搜了搜博客 发现是函数声明 
                          
enum task_status
{
    TASK_RUNNING, // 0
    TASK_READY,   // 1
    TASK_BLOCKED, // 2
    TASK_WAITING, // 3
    TASK_HANGING, // 4
    TASK_DIED     // 5
};

/*         intr_stack 用于处理中断被切换的上下文环境储存 */
/*	   这里我又去查了一下 为什么是反着的 越在后面的参数 地址越高 */

struct intr_stack
{
    uint32_t vec_no; //中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    
    uint32_t err_code;
    void (*eip) (void);        //这里声明了一个函数指针 
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};


/*	线程栈 保护线程环境 */

struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    void (*eip) (thread_func* func,void* func_arg); //和下面的相互照应 以ret 汇编代码进入kernel_thread函数调用
    
    void (*unused_retaddr);                         //占位数 在栈顶站住了返回地址的位置 因为是汇编ret 
    thread_func* function;                          //进入kernel_thread要调用的函数地址
    void* func_arg;				      //参数指针
};

struct task_struct
{
    uint32_t* self_kstack;                          //pcb中的 kernel_stack 内核栈
    pid_t pid;
    enum task_status status;                        //线程状态
    uint8_t priority;				      //特权级
    uint8_t ticks;				      //在cpu 运行的滴答数 看ticks 来判断是否用完了时间片
    uint32_t elapsed_ticks;                         //一共执行了多久
    char name[16];
    
    struct list_elem general_tag;                   //就绪队列中的连接节点
    struct list_elem all_list_tag;		      //总队列的连接节点
    int32_t fd_table[MAX_FILES_OPEN_PER_PROC];      //文件描述符数组
    
    uint32_t* pgdir;				      //进程自己页表的虚拟地址 线程没有   
    struct virtual_addr userprog_vaddr;	      //用户进程的虚拟空间               
    struct mem_block_desc u_block_desc[DESC_CNT];   //内存块描述符
    uint32_t cwd_inode_nr;			      //工作目录inode编号
    int16_t  parent_pid;			      //父进程的pid编号
    uint32_t stack_magic;			      //越界检查  因为我们pcb上面的就是我们要用的栈了 到时候还要越界检查
};

pid_t allocate_pid(void);
struct task_struct* running_thread(void);
void kernel_thread(thread_func* function,void* func_arg);
void thread_create(struct task_struct* pthread,thread_func function,void* func_arg);
void init_thread(struct task_struct* pthread,char* name,int prio);
struct task_struct* thread_start(char* name,int prio,thread_func function,void* func_arg);
void make_main_thread(void);
void schedule(void);
void thread_init(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);
void idle(void);
void thread_yield(void);
pid_t fork_pid(void);
void pad_print(char* buf,int32_t buf_len,void* ptr,char format);
bool elem2thread_info(struct list_elem* pelem,int arg);
void sys_ps(void);

#endif
