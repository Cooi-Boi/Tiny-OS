#include "exec.h"
#include "global.h"
#include "memory.h"
#include "string.h"
#include "../fs/fs.h"
#include "../fs/file.h"
#include "stdio-kernel.h"
#include "interrupt.h"
#include "syscall.h"


extern void intr_exit(void);


/*****************


//半完成品
//值到最后的读取出现了错误 找了一天的原因 也对比了一天的代码了
//值能够读取 但是发生了缺页 遗憾告终



****************/
//将偏移为offset 大小为filesz的段移动到虚拟地址为 vaddr 的内存上
bool segment_load(int32_t fd,uint32_t offset,uint32_t filesz,uint32_t vaddr)
{
    //得到页框起始位置
    uint32_t vaddr_first_page = vaddr & 0xfffff000;
    //得到第一个页面的剩余空间
    uint32_t size_in_first_page = PG_SIZE - (vaddr & 0x00000fff);
    
    //占用页面数
    uint32_t occupy_pages = 0; 
    //大于第一个页面的剩余空间
    if(filesz > size_in_first_page)
    {
        uint32_t left_size = filesz - size_in_first_page;
        occupy_pages = DIV_ROUND_UP(left_size,PG_SIZE) + 1;
    }
    else    occupy_pages = 1;
    
    //开始分配内存咯 如果那个位置的内存没有 我们即分配 有的话直接覆盖即可
    uint32_t page_idx= 0;
    uint32_t vaddr_page = vaddr_first_page;
    
    while(page_idx < occupy_pages)
    {
        uint32_t* pde = pde_ptr(vaddr_page);
        uint32_t* pte = pte_ptr(vaddr_page);
        //先看页目录在不在 在的话再判断页表上面有没有 都没有的话 就直接分配内存即可 毕竟用的是老进程的页表
        if(!(*pde & 0x00000001) || !(*pte & 0x00000001))
        {
            if(get_a_page(PF_USER,vaddr_page) == NULL)
                return false;
        }
        printk("ok!\n");
        //如果原进程页表就分配了 之间覆盖于其上即可
        //不需要客气 哈哈
        vaddr_page += PG_SIZE;
        ++page_idx;
    }
    sys_lseek(fd,offset,SEEK_SET);    //文件指针指向我们的offset
    sys_read(fd,vaddr,filesz); //fd指向的文件
    while(1);
    printk("very ok!\n");
    return true;
}

//在文件系统中 加载用户程序 并返回起始地址
int32_t load(const char* pathname)
{
    int32_t ret = -1;
    struct Elf32_Ehdr elf_header;  //elf头
    struct Elf32_Phdr prog_header; //程序头表
    memset(&elf_header,0,sizeof(struct Elf32_Ehdr));
    
    int32_t fd = open(pathname,O_RDONLY);
    if(fd == -1)
    	return -1;
    
    if(read(fd,&elf_header,sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr))
        goto done;
    sys_lseek(fd,0,SEEK_SET);
    //开头7字节是固定的魔数 0X7F 0X45 0X4C 0X46 0X1 0X1 0X1  我博客里面readelf那张图里面也有
    if(memcmp(elf_header.e_ident,"\177ELF\1\1\1",7) || elf_header.e_machine != 3 || elf_header.e_type != 2 || \
       elf_header.e_version != 1 || elf_header.e_phnum > 1024 || \
       elf_header.e_phentsize != sizeof(struct Elf32_Phdr))
    {
        printk("load: memcmp fail\n");
        goto done;
    }
    //程序头表在文件中的偏移地址
    Elf32_Off  prog_header_offset = elf_header.e_phoff;
    Elf32_Half prog_header_size   = elf_header.e_phentsize;
    
    //遍历程序头 根据e_phnum来遍历 具体偏移位置由程序头表里面的offset决定 offset是给出下个程序头距离当前的头的巨鹿
    uint32_t prog_idx = 0;
    while(prog_idx < elf_header.e_phnum)
    {
        memset(&prog_header,0,sizeof(struct Elf32_Phdr));
        sys_lseek(fd,prog_header_offset,SEEK_SET);
        if(sys_read(fd,&prog_header,prog_header_size) != prog_header_size)
            goto done;
        //可用于加载的有效段
        if(PT_LOAD == prog_header.p_type && prog_header.p_vaddr >= elf_header.e_entry)
        {
            //载入到内存中
            if(!segment_load(fd,prog_header.p_offset,prog_header.p_filesz,prog_header.p_vaddr))
                goto done;
        }
        
        prog_header_offset += elf_header.e_phentsize;
        ++prog_idx;
    }
    
    ret = elf_header.e_entry;  //入口点地址
    done:
    sys_close(fd);
    return ret;
}

//用path指向的程序替换进程
int32_t sys_execv(const char* path,const char* argv[])
{
    uint32_t argc = 0;
    while(argv[argc])
    	++argc;
    	
    int32_t entry_point = load(path);
    if(entry_point == -1)
        return -1;
        
    //得到pcb
    struct task_struct* cur = running_thread();
    int32_t TASK_NAME_LEN = strlen(path);
    memset(cur->name,0,strlen(cur->name));
    memcpy(cur->name,path,TASK_NAME_LEN);
    cur->name[TASK_NAME_LEN-1] = 0;
    
    //这里我们在fork那里 已经在熟悉不过了
    struct intr_stack* intr_0_stack = (struct intr_stack*)\
    ((uint32_t)cur + PG_SIZE - sizeof(struct intr_stack));
    intr_0_stack->ebx = (uint32_t)argv;      // 参数地址 第一个参数位置
    intr_0_stack->ecx = argc;               // 参数个数 第二个参数位置
    intr_0_stack->eip = (void*)entry_point; // 入口地址处 ok了家人
    intr_0_stack->esp = (void*)0xc0000000; //用户栈最顶点 当然我们用户栈也一直设置在那里的 哈哈
    
    //好凶 直接开跳了
    while(1);
    asm volatile("movl %0,%%esp ; jmp intr_exit" : : "g" (intr_0_stack) : "memory");
    return 0;// 哈哈 跳都跳转了 无法执行了
}
