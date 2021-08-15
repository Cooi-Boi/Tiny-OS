#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "debug.h"
#include "string.h"
#include "global.h"
#include "../thread/sync.h"
#include "../thread/thread.h"
#include "../lib/kernel/list.h"
#include "interrupt.h"

#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0Xc009a000   //位图开始存放的位置
#define K_HEAP_START    0xc0100000   //内核栈起始位置
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool
{
    struct bitmap pool_bitmap; //位图来管理内存使用
    uint32_t phy_addr_start;   //内存池开始的起始地址
    uint32_t pool_size;	//池容量
    struct lock lock;
};
struct mem_block_desc k_block_descs[DESC_CNT]; //内核内存块描述符数组

struct pool kernel_pool ,user_pool; //生成内核内存池 和 用户内存池
struct virtual_addr kernel_vaddr;    //内核虚拟内存管理池

void* vaddr_get(enum pool_flags pf,uint32_t pg_cnt)
{
    int vaddr_start = 0,bit_idx_start = -1;
    uint32_t cnt = 0;
    if(pf == PF_KERNEL)
    {
    	bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap,pg_cnt);
    	if(bit_idx_start == -1)	return NULL;
    	while(cnt < pg_cnt)
    	    bitmap_set(&kernel_vaddr.vaddr_bitmap,bit_idx_start + (cnt++),1);
    	vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
    	struct task_struct* cur = running_thread();
    	bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap,pg_cnt);
    	if(bit_idx_start == -1)	return NULL;
    	
    	while(cnt < pg_cnt)
    	    bitmap_set(&cur->userprog_vaddr.vaddr_bitmap,bit_idx_start + (cnt++),1);
    	vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    	ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*)vaddr_start;
}

uint32_t* pte_ptr(uint32_t vaddr)
{
    uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

uint32_t* pde_ptr(uint32_t vaddr)
{
    uint32_t* pde = (uint32_t*) ((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}

void* palloc(struct pool* m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap,1);
    if(bit_idx == -1)	return NULL;
    bitmap_set(&m_pool->pool_bitmap,bit_idx,1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*)page_phyaddr;
}

void page_table_add(void* _vaddr,void* _page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr,page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);
    
    if(*pde & 0x00000001)
    {
    	ASSERT(!(*pte & 0x00000001));
    	if(!(*pte & 0x00000001))
    	    *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    	else
    	{
    	    PANIC("pte repeat");
    	    *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    	}
    } 
    else
    {
    	uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
    	*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    	memset((void*)((int)pte & 0xfffff000),0,PG_SIZE);
    	ASSERT(!(*pte & 0x00000001));
    	*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
    return;
}

void* malloc_page(enum pool_flags pf,uint32_t pg_cnt)
{
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    
    void* vaddr_start = vaddr_get(pf,pg_cnt);
    if(vaddr_start == NULL)	return NULL;
    
    
    uint32_t vaddr = (uint32_t)vaddr_start,cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    
    while(cnt-- > 0)
    {
    	void* page_phyaddr = palloc(mem_pool);
    	if(page_phyaddr == NULL)	return NULL;
    	page_table_add((void*)vaddr,page_phyaddr);
    	vaddr += PG_SIZE;
    }
    return vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt)
{
    void* vaddr = malloc_page(PF_KERNEL,pg_cnt);
    if(vaddr != NULL)	memset(vaddr,0,pg_cnt*PG_SIZE);
    return vaddr;
}

void* get_user_pages(uint32_t pg_cnt)
{
    lock_acquire(&user_pool.lock);	//用户进程可能会产生冲突 大部分时间都在用户进程 内核进程可以理解基本不会冲突
    void* vaddr = malloc_page(PF_USER,pg_cnt);
    if(vaddr != NULL)	memset(vaddr,0,pg_cnt*PG_SIZE);
    lock_release(&user_pool.lock);
    return vaddr;
}

void* get_a_page(enum pool_flags pf,uint32_t vaddr)
{
    struct pool* mem_pool = (pf == PF_KERNEL) ? &kernel_pool : &user_pool;
    lock_acquire(&mem_pool->lock);
    
    struct task_struct* cur = running_thread();
    int32_t bit_idx = -1;
    
    //虚拟地址位图置1
    if(cur->pgdir != NULL && pf == PF_USER)
    {
    	bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
    	ASSERT(bit_idx > 0);
    	bitmap_set(&cur->userprog_vaddr.vaddr_bitmap,bit_idx,1);
    }
    else if(cur->pgdir == NULL && pf == PF_KERNEL) 
    {
    	bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
    	ASSERT(bit_idx > 0);
    	bitmap_set(&kernel_vaddr.vaddr_bitmap,bit_idx,1);
    }
    else
    	PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
    	
    void* page_phyaddr = palloc(mem_pool);
    if(page_phyaddr == NULL)
    	return NULL;
    page_table_add((void*)vaddr,page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

/* 得到虚拟地址映射的物理地址 */
uint32_t addr_v2p(uint32_t vaddr)
{
    uint32_t* pte = pte_ptr(vaddr);
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

void mem_pool_init(uint32_t all_mem)
{
    put_str("    mem_pool_init start!\n");
    uint32_t page_table_size = PG_SIZE * 256;       //页表占用的大小
    uint32_t used_mem = page_table_size + 0x100000; //低端1MB的内存 + 页表所占用的大小
    uint32_t free_mem = all_mem - used_mem;
    
    uint16_t all_free_pages = free_mem / PG_SIZE;   //空余的页数 = 总空余内存 / 一页的大小
    
    uint16_t kernel_free_pages = all_free_pages /2; //内核 与 用户 各平分剩余内存
    uint16_t user_free_pages = all_free_pages - kernel_free_pages; //万一是奇数 就会少1 减去即可
    
    //kbm kernel_bitmap ubm user_bitmap
    uint32_t kbm_length = kernel_free_pages / 8;    //一位即可表示一页 8位一个数
    uint32_t ubm_length = user_free_pages / 8;
    
    //kp kernel_pool up user_pool
    uint32_t kp_start = used_mem;
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;
    
    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;
    
    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;
    
    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);
    
    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length; 
    
    put_str("        kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_char('\n');
    put_str("        user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_char('\n');
    
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);
    
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    
    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);
    put_str("    mem_pool_init done\n");
    return;
}

void block_desc_init(struct mem_block_desc* desc_array)
{
    uint16_t desc_idx,block_size = 16;
    for(desc_idx = 0;desc_idx < DESC_CNT;desc_idx++)
    {
    	desc_array[desc_idx].block_size = block_size;
    	desc_array[desc_idx].block_per_arena = (PG_SIZE - sizeof(struct arena)) / block_size;
    	list_init(&desc_array[desc_idx].free_list);
    	block_size *= 2;
    }   
}

void mem_init()
{
    put_str("mem_init start!\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0x800)); //我们把总内存的值放在了0x800 我之前为了显示比较独特 放在了0x800处了
    mem_pool_init(mem_bytes_total);
    block_desc_init(k_block_descs);
    put_str("mem_init done!\n");
    return;
}

//返回idx个内存块的地址 空间布局为 arena元信息+n个内存块
struct mem_block* arena2block(struct arena* a,uint32_t idx)
{
    return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

//自然页框的最低点处
struct arena* block2arena(struct mem_block* b)
{
    return (struct arena*)((uint32_t)b & 0xfffff000);
}

void* sys_malloc(uint32_t size)
{
    enum pool_flags PF;
    struct pool* mem_pool;
    uint32_t pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();
    
    if(cur_thread->pgdir == NULL)
    {
    	PF = PF_KERNEL;
    	pool_size = kernel_pool.pool_size;
    	mem_pool = &kernel_pool;
    	descs = k_block_descs;
    }
    else
    {
    	PF = PF_USER;
    	pool_size = user_pool.pool_size;
    	mem_pool = &user_pool;
    	descs = cur_thread->u_block_desc;
    }
    //size 超出空间大小或者为 0 负数
    if(!(size > 0 && size < pool_size))
    {
    	return NULL;
    }
    
    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);
    
    //超过页框直接分配页框
    if(size > 1024)
    {
    	uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena),PG_SIZE);
    	a = malloc_page(PF,page_cnt);
    	if(a != NULL)
    	{
    	    memset(a,0,page_cnt * PG_SIZE);
	    a->desc = NULL;
	    a->cnt  = page_cnt;
	    a->large = true;    
	    lock_release(&mem_pool->lock);
	    return (void*)(a+1); //返回地址 struct arena + sizeof(struct arena)
	}
	else
	{
	    lock_release(&mem_pool->lock);
	    return NULL;
	}
    }
    else
    {
    	uint8_t desc_idx;
    	for(desc_idx = 0;desc_idx < DESC_CNT;desc_idx++)
    	{
    	    if(size <= descs[desc_idx].block_size)
    	    {
    	    	break;
    	    }
    	}
    	
    	//已经空了
    	if(list_empty(&descs[desc_idx].free_list))
    	{
    	    a = malloc_page(PF,1);
    	    if(a == NULL)
    	    {
    	    	lock_release(&mem_pool->lock);
    	    	return NULL;
    	    }
    	    memset(a,0,PG_SIZE);
    	    
    	    a->desc = &descs[desc_idx];
    	    a->large = false;
    	    a->cnt = descs[desc_idx].block_per_arena;
    	    uint32_t block_idx;
    	    
    	    enum intr_status old_status = intr_disable();
    	    
    	    for(block_idx = 0;block_idx < descs[desc_idx].block_per_arena;++block_idx)
    	    {
    	    	b = arena2block(a,block_idx);
    	    	ASSERT(!elem_find(&a->desc->free_list,&b->free_elem));
    	    	list_append(&a->desc->free_list,&b->free_elem);
    	    }
    	    intr_set_status(old_status);
    	}
    	b = elem2entry(struct mem_block,free_elem,list_pop(&(descs[desc_idx].free_list)));
    	memset(b,0,descs[desc_idx].block_size);
    	
    	a = block2arena(b);
    	--a->cnt;
    	lock_release(&mem_pool->lock);
    	return (void*)b;
    }
}

void pfree(uint32_t pg_phy_addr)
{
    struct pool* mem_pool;
    uint32_t bit_idx = 0;
    if(pg_phy_addr >= user_pool.phy_addr_start)
    {
    	mem_pool = &user_pool;
    	bit_idx = (pg_phy_addr - user_pool.phy_addr_start) / PG_SIZE;
    }
    else
    {
    	mem_pool = &kernel_pool;
    	bit_idx = (pg_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap,bit_idx,0);	//全部置0
}

void page_table_pte_remove(uint32_t vaddr)
{
    uint32_t* pte = pte_ptr(vaddr);
    *pte = ~PG_P_1;
    asm volatile("invlpg %0"::"m"(vaddr):"memory");	//更新tlb
}

void vaddr_remove(enum pool_flags pf,void* _vaddr,uint32_t pg_cnt)
{
    uint32_t bit_idx_start = 0,vaddr = (uint32_t)_vaddr,cnt = 0;
    
    if(pf == PF_KERNEL)
    {
    	bit_idx_start = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
    	while(cnt < pg_cnt)
    	{
    	    bitmap_set(&kernel_vaddr.vaddr_bitmap,bit_idx_start + (cnt++),0);
    	}
    }
    else
    {
    	struct task_struct* cur_thread = running_thread();
    	bit_idx_start = (vaddr - cur_thread->userprog_vaddr.vaddr_start) / PG_SIZE;
    	while(cnt < pg_cnt)
    	{
    	    bitmap_set(&cur_thread->userprog_vaddr.vaddr_bitmap,bit_idx_start + (cnt++),0);
    	}
    }
}

void mfree_page(enum pool_flags pf,void* _vaddr,uint32_t pg_cnt)
{
    uint32_t pg_phy_addr;
    uint32_t vaddr = (int32_t)_vaddr,page_cnt = 0;
    ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
    pg_phy_addr = addr_v2p(vaddr);
    
    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000);
    
    if(pg_phy_addr >= user_pool.phy_addr_start)
    {
    	vaddr -= PG_SIZE;
    	while(page_cnt < pg_cnt)
    	{
    	    vaddr += PG_SIZE;
    	    pg_phy_addr = addr_v2p(vaddr);
    	    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= user_pool.phy_addr_start);
    	    pfree(pg_phy_addr);
    	    page_table_pte_remove(vaddr);
    	    page_cnt++;
    	}
    	vaddr_remove(pf,_vaddr,pg_cnt);
   }
   else
   {
   	vaddr -= PG_SIZE;
   	while(page_cnt < pg_cnt)
   	{
   	    vaddr += PG_SIZE;
   	    pg_phy_addr = addr_v2p(vaddr);
   	    ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= kernel_pool.phy_addr_start && pg_phy_addr < user_pool.phy_addr_start);
   	    pfree(pg_phy_addr);
   	    page_table_pte_remove(vaddr);
   	    page_cnt++;
   	}
   	vaddr_remove(pf,_vaddr,pg_cnt);
   }
}

void sys_free(void* ptr)
{
    ASSERT(ptr != NULL);
    if(ptr != NULL)
    {
    	enum pool_flags PF;
    	struct pool* mem_pool;
    	
    	if(running_thread()->pgdir == NULL)
    	{
    	    ASSERT((uint32_t)ptr >= K_HEAP_START);
    	    PF = PF_KERNEL;
    	    mem_pool = &kernel_pool;
    	}
    	else
    	{
    	    PF = PF_USER;
    	    mem_pool = &user_pool;
    	}
    	
    	lock_acquire(&mem_pool->lock);
    	struct mem_block* b = ptr;
    	struct arena* a = block2arena(b);
    	
    	ASSERT(a->large == 0 || a->large == 1);
    	if(a->desc == NULL && a->large == true)
    	{
    	    mfree_page(PF,a,a->cnt);
    	}
    	else
    	{
    	    list_append(&a->desc->free_list,&b->free_elem);
    	    if(++a->cnt == a->desc->block_per_arena)
    	    {
    	    	uint32_t block_idx;
    	    	for(block_idx = 0;block_idx < a->desc->block_per_arena;block_idx++)
    	    	{
    	    	    struct mem_block* b = arena2block(a,block_idx);
    	    	    ASSERT(elem_find(&a->desc->free_list,&b->free_elem));
    	    	    list_remove(&b->free_elem);
    	    	}
    	    	mfree_page(PF,a,1);
    	    }
    	}
    	lock_release(&mem_pool->lock);
    }
}

//得到1页大小内存并复制到页表中 专门针对fork时虚拟地址位图无需操作
//因为位图我们后面会复制父进程的 所以当然不用继续对虚拟位图操作了
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf,uint32_t vaddr)
{
    struct pool* mem_pool = (pf == PF_KERNEL) ? &kernel_pool : &user_pool;
    lock_acquire(&mem_pool->lock);
    void* page_phyaddr = palloc(mem_pool);
    //分配失败
    if(page_phyaddr == NULL)
    {
        lock_release(&mem_pool->lock);
        return NULL;
    }
    page_table_add((void*)vaddr,page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}
