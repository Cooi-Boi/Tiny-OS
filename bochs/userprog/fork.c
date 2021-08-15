#include "fork.h"
#include "global.h"
#include "stdint.h"
#include "string.h"
#include "memory.h"
#include "interrupt.h"
#include "../thread/sync.h"
#include "../thread/thread.h"
#include "debug.h"
#include "process.h"
#include "stdio-kernel.h"
#include "../fs/file.h"
#include "list.h"

extern void intr_exit(void);

//复制父进程的pcb
int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread,struct task_struct* parent_thread)
{
    //直接先把pcb所在页 包括内核栈 中断栈全部一起复制过来 其他的需要修改的再一项项改
    memcpy(child_thread,parent_thread,PG_SIZE);
    child_thread->pid = fork_pid();
    child_thread->elapsed_ticks = 0;
    child_thread->status = TASK_READY; //之后要放到就绪队列 被调度的
    child_thread->ticks = child_thread->priority; //时间片填满
    child_thread->parent_pid = parent_thread->pid; //默认是-1 对于子进程的parent_pid 父进程的pid
    child_thread->general_tag.prev = child_thread->general_tag.next = NULL; 
    child_thread->all_list_tag.prev = child_thread->all_list_tag.next = NULL;
    block_desc_init(child_thread->u_block_desc);	//malloc 内存块分配符初始化
    //虚拟位图需要分配页 的页数 毕竟两个进程不能共享虚拟内存位图嘛 但是我们是需要把父进程的给复制了 
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8 , PG_SIZE);  
    void* vaddr_btmp = get_kernel_pages(bitmap_pg_cnt);
    //复制父进程的虚拟内存位图 并把自己刚分配好的独立位图给我们的子进程赋值
    memcpy(vaddr_btmp,child_thread->userprog_vaddr.vaddr_bitmap.bits,bitmap_pg_cnt * PG_SIZE);
    child_thread->userprog_vaddr.vaddr_bitmap.bits = vaddr_btmp;
    ASSERT(strlen(child_thread->name) < 11); //进程后面加个名字
    strcat(child_thread->name,"_fork");
    return 0;
}

//设置子进程的进程体 用户栈
//把父进程的内存区全部复制下来 buf_page是因为用户进程间无法共享内存 看不见彼此 只能通过buf_page来作为过渡
void copy_body_stack3(struct task_struct* parent_thread,struct task_struct* child_thread,void* buf_page)
{
    uint8_t* vaddr_btmp = parent_thread->userprog_vaddr.vaddr_bitmap.bits;
    uint32_t btmp_bytes_len = parent_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_len;
    uint32_t vaddr_start = parent_thread->userprog_vaddr.vaddr_start;
    uint32_t idx_byte = 0;
    uint32_t idx_bit = 0;
    uint32_t prog_vaddr = 0;
    
    //根据虚拟内存位图来看 我们只需要把位图中看看哪些页被用了
    //我们把那些页给复制过去即可 同时也需要把页表在新进程中安装一下
    while(idx_byte < btmp_bytes_len)
    {
        if(vaddr_btmp[idx_byte])
        {
            idx_bit = 0;
            while(idx_bit < 8) //一字节8位
            {
                if((BITMAP_MASK << idx_bit) & vaddr_btmp[idx_byte])
                {
                    prog_vaddr = (idx_byte * 8 + idx_bit) * PG_SIZE + vaddr_start;
                    memcpy(buf_page,(void*)prog_vaddr,PG_SIZE);
                    page_dir_activate(child_thread); //切换到用户页表 防止安装到父进程里面去了
                    get_a_page_without_opvaddrbitmap(PF_USER,prog_vaddr);
                    memcpy((void*)prog_vaddr,buf_page,PG_SIZE);
                    page_dir_activate(parent_thread); //切换回父进程
                }
                ++idx_bit;
            }
        }
        ++idx_byte;
    }    
}

//给子进程构建内核栈 且设置0 返回值
int32_t build_child_stack(struct task_struct* child_thread)
{
    //内核栈的最高地址处 intr中断栈最低地址处
    struct intr_stack* intr_0_stack = \
      (struct intr_stack*)((uint32_t)child_thread + PG_SIZE - sizeof(struct intr_stack));
    //返回值 0
    intr_0_stack->eax = 0;
    //这里我把其结构体搬过来了
    /*struct thread_stack
    {
        uint32_t ebp;
        uint32_t ebx;
        uint32_t edi;
        uint32_t esi;

        void (*eip) (thread_func* func,void* func_arg); //和下面的相互照应 以ret 汇编代码进入kernel_thread函数调用
    
        void (*unused_retaddr);                         //占位数 在栈顶站住了返回地址的位置 因为是汇编ret 
        thread_func* function;                          //进入kernel_thread要调用的函数地址
        void* func_arg;				      //参数指针
    };*/
    //返回地址毕竟是高地址
    uint32_t* ret_addr_in_thread_stack = (uint32_t*)intr_0_stack - 1;
    uint32_t* esi_ptr_in_thread_stack =  (uint32_t*)intr_0_stack - 2;
    uint32_t* edi_ptr_in_thread_stack =  (uint32_t*)intr_0_stack - 3;
    uint32_t* ebx_ptr_in_thread_stack =  (uint32_t*)intr_0_stack - 4;
    uint32_t* ebp_ptr_in_thread_stack =  (uint32_t*)intr_0_stack - 5;
    
    *ret_addr_in_thread_stack = (uint32_t)intr_exit;
    //反正之后的pop都会覆盖
    *esi_ptr_in_thread_stack = *edi_ptr_in_thread_stack = *ebx_ptr_in_thread_stack = \
    *ebp_ptr_in_thread_stack = 0;
    //内核栈栈顶
    child_thread->self_kstack = ebp_ptr_in_thread_stack;
    return 0;
}

//更新inode打开数
void updata_inode_open_cnts(struct task_struct* thread)
{
    int32_t local_fd = 3,global_fd = 0;
    while(local_fd < MAX_FILES_OPEN_PER_PROC)
    {
        global_fd = thread->fd_table[local_fd];
        ASSERT(global_fd < MAX_FILE_OPEN);
        if(global_fd != -1)
            ++file_table[global_fd].fd_inode->i_open_cnts;
        ++local_fd;
    }
}

//汇总函数 包装 把父进程资源给子进程
int32_t copy_process(struct task_struct* child_thread,struct task_struct* parent_thread)
{
    //用于给memcpy 过渡的页面
    void* buf_page = get_kernel_pages(1);
    if(buf_page == NULL)
    {
        printk("copy_process: buf_page alloc fail\n");
        return -1;
    }
    if(copy_pcb_vaddrbitmap_stack0(child_thread,parent_thread) == -1)
    {
        printk("copy_process: copy_pcb_vaddrbitmap_stack0 fail\n");
        return -1;
    }
    
    child_thread->pgdir = create_page_dir();
    if(child_thread->pgdir == NULL)
    {
        printk("copy_process: child_thread->pgdir create fail\n");
    	return -1;
    }
    
    copy_body_stack3(parent_thread,child_thread,buf_page);
    build_child_stack(child_thread);
    updata_inode_open_cnts(child_thread);
    mfree_page(PF_KERNEL,buf_page,1);
    return 0;
}

//禁止从内核调用 只能从用户进程调用
pid_t sys_fork(void)
{
    struct task_struct* parent_thread = running_thread();
    struct task_struct* child_thread  = get_kernel_pages(1);
    if(child_thread == NULL)
    	return -1;
    
    ASSERT(INTR_OFF == intr_get_status() && parent_thread->pgdir != NULL);
    
    if(copy_process(child_thread,parent_thread) == -1)
    	return -1;
    	
    ASSERT(!elem_find(&thread_ready_list,&child_thread->general_tag));
    list_append(&thread_ready_list,&child_thread->general_tag);
    ASSERT(!elem_find(&thread_all_list,&child_thread->all_list_tag));
    list_append(&thread_all_list,&child_thread->all_list_tag);
    
    return child_thread->pid;
}
