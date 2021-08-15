#include "inode.h"
#include "ide.h"
#include "debug.h"
#include "../thread/thread.h"
#include "memory.h"
#include "string.h"
#include "list.h"
#include "interrupt.h"
#include "bitmap.h"
#include "file.h"

//得到inode 所在位置 给inode_pos赋值
void inode_locate(struct partition* part,uint32_t inode_no,struct inode_position* inode_pos)
{
    ASSERT(inode_no < 4096);
    uint32_t inode_table_lba = part->sb->inode_table_lba; //得到起始扇区lba
    
    uint32_t inode_size = sizeof(struct inode);
    uint32_t off_size = inode_no * inode_size;
    uint32_t off_sec  = off_size / 512;		    //得到偏移扇区数
    uint32_t off_size_in_sec = off_size % 512;	    //得到在扇区内偏移字节数
    
    uint32_t left_in_sec = 512 - off_size_in_sec;	    //在扇区内剩余字节数 不满sizeof(struct inode)即在两扇区间
    if(left_in_sec < inode_size)	inode_pos->two_sec = true;
    else       inode_pos->two_sec = false;
    
    inode_pos->sec_lba = inode_table_lba + off_sec;
    inode_pos->off_size = off_size_in_sec;
}

//把inode写回硬盘 io_buf是提前申请的缓冲区 防止内存申请失败前面回读状态时前面任务全白做
void inode_sync(struct partition* part,struct inode* inode,void* io_buf)
{
    uint8_t inode_no = inode->i_no;			
    struct inode_position inode_pos;			    //inode_position
    inode_locate(part,inode_no,&inode_pos);
    
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));
    
    struct inode pure_inode;
    memcpy(&pure_inode,inode,sizeof(struct inode));
    
    //这些都只在内存中有用
    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny  = false;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;
    
    char* inode_buf = (char*)io_buf;
    //如果跨分区
    if(inode_pos.two_sec)
    {
    	//一次性读两个扇区的内容 复制粘贴完了回读回去
    	ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,2);
    	memcpy((inode_buf + inode_pos.off_size),&pure_inode,sizeof(struct inode));
    	ide_write(part->my_disk,inode_pos.sec_lba,inode_buf,2);
    }
    else
    {
    	ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,1);
    	memcpy((inode_buf + inode_pos.off_size),&pure_inode,sizeof(struct inode));
    	ide_write(part->my_disk,inode_pos.sec_lba,inode_buf,1);
    }
}

//返回相对应的inode指针
struct inode* inode_open(struct partition* part,uint32_t inode_no)
{
    //先从inode链表找到inode 为了提速而创建的
    struct list_elem* elem = part->open_inodes.head.next; //第一个结点开始
    struct inode* inode_found;			    //返回
    while(elem != &part->open_inodes.tail)
    {
    	inode_found = elem2entry(struct inode,inode_tag,elem);
        if(inode_found->i_no == inode_no)
        {
            inode_found->i_open_cnts++;
            return inode_found;
        }
    	elem = elem->next;
    }
    
    //缓冲链表没找到 则需要在硬盘区找
    struct inode_position inode_pos;
    inode_locate(part,inode_no,&inode_pos);
    
    //由于我们的sys_malloc 看当前线程处于用户还是内核是根据 pgdir是否有独立页表 
    //而不管是我们处在用户还是内核 为了让所有进程都能看到这个inode 我们必须把内存分配在内核
    struct task_struct* cur = running_thread();
    uint32_t* cur_pagedir_back = cur->pgdir;
    cur->pgdir = NULL; 
    inode_found = (struct inode*)sys_malloc(sizeof(struct inode));
    cur->pgdir = cur_pagedir_back;
    
    char* inode_buf;
    if(inode_pos.two_sec)
    {
        inode_buf = (char*)sys_malloc(1024);
        ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,2);
    }
    else
    {
    	inode_buf = (char*)sys_malloc(512);
        ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,1);
    }
    memcpy(inode_found,inode_buf + inode_pos.off_size,sizeof(struct inode));   
    //放在队首 因为很有可能接下来要访问
    list_push(&part->open_inodes,&inode_found->inode_tag);
    inode_found->i_open_cnts = 1;
    
    sys_free(inode_buf);
    return inode_found;
}

//关闭inode 或减少inode打开数
void inode_close(struct inode* inode)
{
    enum intr_status old_status = intr_disable();
    //减少次数后 当发现为0了 直接释放即可
    if(--inode->i_open_cnts == 0)
    {
    	struct task_struct* cur = running_thread();
    	uint32_t* cur_pagedir_back = cur->pgdir;
    	cur->pgdir = NULL;
    	sys_free(inode);
    	cur->pgdir = cur_pagedir_back;
    }
    intr_set_status(old_status);
}

//初始化inode inode第一个编号是0
void inode_init(uint32_t inode_no,struct inode* new_inode)
{
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;
    
    uint8_t sec_idx = 0;
    while(sec_idx < 13)
    	new_inode->i_sectors[sec_idx++] = 0;
}

//清除inode_table中的inode
void inode_delete(struct partition* part,uint32_t inode_no,void* io_buf)
{
    ASSERT(inode_no < 4096);
    struct inode_position inode_pos;	//存储inode的pos的
    inode_locate(part,inode_no,&inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));
    
    char* inode_buf = (char*)io_buf;
    if(inode_pos.two_sec)
    {
        ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,2);	//夹在两个扇区间
        memset((inode_buf + inode_pos.off_size),0,sizeof(struct inode)); //把inode信息清除 再次写回硬盘
        ide_write(part->my_disk,inode_pos.sec_lba,inode_buf,2); 
    }
    else
    {
        ide_read(part->my_disk,inode_pos.sec_lba,inode_buf,1);	//夹在两个扇区间
        memset((inode_buf + inode_pos.off_size),0,sizeof(struct inode)); //把inode信息清除 再次写回硬盘
        ide_write(part->my_disk,inode_pos.sec_lba,inode_buf,1); 
    }
}

//回收inode 将inode位图给收回 并把inode里面所占用的块给回收
void inode_release(struct partition* part,uint32_t inode_no)
{
    struct inode* inode_to_del = inode_open(part,inode_no);  //返回inode指针
    ASSERT(inode_to_del->i_no == inode_no);
    
    uint8_t  block_idx = 0,block_cnt = 12; //预设先设置12个直接块
    uint32_t block_bitmap_idx;
    uint32_t all_blocks[140] = {0};	//所有块的块地址 12 + 128 = 140
    
    //先把12个直接块地址放进去 再看间接块是否为0 为0无视 有地址的话 再把间接块全部放进去
    while(block_idx < 12)
    {
    	all_blocks[block_idx] = inode_to_del->i_sectors[block_idx];
    	++block_idx;
    }
    
    if(inode_to_del->i_sectors[12] != 0)
    {
        ide_read(part,inode_to_del->i_sectors[12],all_blocks + 12,1);
        block_cnt = 140;
        
        //先回收存放间接块地址的那一块
        block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx > 0);
        bitmap_set(&part->block_bitmap,block_bitmap_idx,0);
        bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    }
    
    block_idx = 0;
    
    while(block_idx < block_cnt)
    {
        if(all_blocks[block_idx] != 0)
        {
        
            //不需要清除硬盘上的0 以后再分配上去 数据覆盖即可
            block_bitmap_idx = 0;
            block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
            ASSERT(block_bitmap_idx > 0);
            bitmap_set(&part->block_bitmap,block_bitmap_idx,0);
            bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
        }
        ++block_idx;
    }
    
    //清除这个inode所占用的inode位图 置0
    bitmap_set(&part->inode_bitmap,inode_no,0);
    bitmap_sync(cur_part,inode_no,INODE_BITMAP);
    
    //下面的inode_delete 主要看看 inode_table 我们原来分配的内容有没有置0
    void* io_buf = sys_malloc(1024);
    inode_delete(part,inode_no,io_buf);
    sys_free(io_buf);    
    inode_close(inode_to_del);  //把内存中的inode删除
}
