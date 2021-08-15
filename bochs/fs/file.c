#include "file.h"
#include "stdio-kernel.h"
#include "../thread/thread.h"
#include "ide.h"
#include "fs.h"
#include "memory.h"
#include "string.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"

struct file file_table[MAX_FILE_OPEN];

//全局文件表中获取
uint32_t get_free_slot_in_global(void)
{
    uint32_t fd_idx = 3;
    while(fd_idx < MAX_FILE_OPEN)
    {
    	if(file_table[fd_idx].fd_inode == NULL)
    		break;
    	++fd_idx;
    }
    
    if(fd_idx == MAX_FILE_OPEN)
    {
    	printk("exceed max open files\n");
    	return -1;
    }
    
    return fd_idx;
}

//安装到pcb中的文件描述符数组中
uint32_t pcb_fd_install(int32_t global_fd_idx)
{
    struct task_struct* cur = running_thread();
    uint8_t local_fd_idx = 3;			//从3开始
    while(local_fd_idx < MAX_FILES_OPEN_PER_PROC)
    {
    	if(cur->fd_table[local_fd_idx] == -1)
    	{
    	    cur->fd_table[local_fd_idx]  = global_fd_idx;
    	    break;
    	}
    	++local_fd_idx;
    }
    
    if(local_fd_idx == MAX_FILES_OPEN_PER_PROC)
    {
    	printk("exceed max open files\n");
    	return -1;
    }
    return local_fd_idx;
}

//分配一个inode 并且返回其结点号
int32_t inode_bitmap_alloc(struct partition* part)
{
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap,1);
    if(bit_idx == -1)  return -1;
    bitmap_set(&part->inode_bitmap,bit_idx,1);
    return bit_idx;
}

//分配一个扇区 并且返回扇区号
int32_t block_bitmap_alloc(struct partition* part)
{
    int32_t bit_idx = bitmap_scan(&part->block_bitmap,1);
    if(bit_idx == -1)	return -1;
    bitmap_set(&part->block_bitmap,bit_idx,1);
    return part->sb->data_start_lba + bit_idx;
}

//将内存中的bitmap第bit_idx位所在的512字节同步到硬盘
void bitmap_sync(struct partition* part,uint32_t bit_idx,uint8_t btmp)
{	
    uint32_t off_sec = bit_idx / 4096;
    uint32_t off_size = off_sec * 512;
    
    uint32_t sec_lba;
    uint8_t* bitmap_off;
    
    switch(btmp)
    {
    	case INODE_BITMAP:
	    sec_lba = part->sb->inode_bitmap_lba + off_sec;
	    bitmap_off = part->inode_bitmap.bits + off_size;
	    break;
	case BLOCK_BITMAP:
	    sec_lba = part->sb->block_bitmap_lba + off_sec;
	    bitmap_off = part->block_bitmap.bits + off_size;  
	    break;
    }
    ide_write(part->my_disk,sec_lba,bitmap_off,1);
}

/*创建文件 成功则返回文件描述符*/
int32_t file_create(struct dir* parent_dir,char* filename,uint8_t flag)
{
    void* io_buf = sys_malloc(1024);
    if(io_buf == NULL)
    {
    	printk("in file_create: sys_malloc for io_buf failed\n");
    	return -1;
    }
    
    uint8_t rollback_step = 0;	// 用于回滚资源状态计数
    int32_t inode_no = inode_bitmap_alloc(cur_part);
    
    if(inode_no == -1)
    {
    	printk("in file_create: allocate inode failed\n");
    	return -1;
    }
    
    struct inode* new_file_inode = (struct inode*)sys_malloc(sizeof(struct inode));
    if(new_file_inode == NULL)
    {
    	printk("file_create: sys_malloc for inode failed\n");
    	rollback_step = 1;
    	goto rollback;
    }
    inode_init(inode_no,new_file_inode);
    
    int fd_idx = get_free_slot_in_global();
    if(fd_idx == -1)
    {
    	printk("exceed max open files\n");
    	rollback_step = 2;
    	goto rollback;
    }
    
    
    file_table[fd_idx].fd_inode = new_file_inode;
    file_table[fd_idx].fd_inode->write_deny = false;
    file_table[fd_idx].fd_pos   = 0;
    file_table[fd_idx].fd_flag  = flag;
    
    struct dir_entry new_dir_entry;
    memset(&new_dir_entry,0,sizeof(struct dir_entry));
    
    create_dir_entry(filename,inode_no,FT_REGULAR,&new_dir_entry);
    
    if(!sync_dir_entry(parent_dir,&new_dir_entry,io_buf))
    {
    	printk("sync dir_entry to disk failed\n");
    	rollback_step = 3;
    	goto rollback;
    }
    
    memset(io_buf,0,1024);
    inode_sync(cur_part,parent_dir->inode,io_buf);
    
    memset(io_buf,0,1024);
    inode_sync(cur_part,new_file_inode,io_buf);
    
    bitmap_sync(cur_part,inode_no,INODE_BITMAP);
    list_push(&cur_part->open_inodes,&new_file_inode->inode_tag);
    new_file_inode->i_open_cnts = 1;
    
    sys_free(io_buf);
    return pcb_fd_install(fd_idx);
    
rollback:
    switch(rollback_step)	//从上到下执行
    {
        case 3:
            memset(&file_table[fd_idx],0,sizeof(struct file));
        case 2:
            sys_free(new_file_inode);
        case 1:
            bitmap_set(&cur_part->inode_bitmap,inode_no,0);
            break;
    }
    sys_free(io_buf);
    return -1;
}

//打开编号为inode_no的文件 成功返回文件描述符 否则返回-1
int32_t file_open(uint32_t inode_no,uint8_t flag)
{
    
    int fd_idx = get_free_slot_in_global();	//全局描述符表得到idx
    if(fd_idx == -1)
    {
    	printk("exceed max open files\n");
    	return -1;
    }
    file_table[fd_idx].fd_pos = 0;
    file_table[fd_idx].fd_flag = flag;
    file_table[fd_idx].fd_inode = inode_open(cur_part,inode_no);
    		  
    bool* write_deny = &file_table[fd_idx].fd_inode->write_deny;
    
    //可写 或者 可读可写 涉及临界区的情况 记得屏蔽中断
    if(flag & O_WRONLY || flag & O_RDWR)
    {
    	enum intr_status old_status = intr_disable();
    	if(!(*write_deny))
    	{
    	    *write_deny = true;
    	    intr_set_status(old_status);
    	}
    	else
    	{
    	    printk("file cant written now try again later!\n");
    	    intr_set_status(old_status);
    	    return -1;
    	}
    }
    return pcb_fd_install(fd_idx);
}

//关闭文件 且使文件描述符恢复可用
int32_t file_close(struct file* file)
{
    if(file == NULL)
        return -1;
    file->fd_inode->write_deny = false;
    inode_close(file->fd_inode);
    file->fd_inode = NULL;	//检测全局描述符u表是否可用 是看inode是否为NULL 即恢复可用
    return 0;
}

int32_t file_read(struct file* file,void* buf,uint32_t count)
{
    uint8_t* buf_dst = (uint8_t*)buf;
    uint32_t size = count ,size_left = count;
    
    //如果要读取的内容比原本文件的字节数更多 则重新设置size 如果到底了 就直接退出
    if((file->fd_pos + count) > file->fd_inode->i_size)
    {
        size = file->fd_inode->i_size - file->fd_pos;
        size_left = size;
        if(size == 0)
            return -1;
    }
    
    uint8_t* io_buf = sys_malloc(BLOCK_SIZE);
    if(io_buf == NULL)
    {
        printk("file_read: sys_malloc for io_buf failed\n");
        return -1;	//书上没写
    }
    
    uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);//48 + 512 = 560
    if(all_blocks == NULL)
    {
        printk("file_read: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint32_t block_read_start_idx = file->fd_pos / BLOCK_SIZE;           //开始读取的扇区号
    uint32_t block_read_end_idx   = (file->fd_pos + count) / BLOCK_SIZE; //结束读取的扇区号
    uint32_t read_blocks          = block_read_end_idx - block_read_start_idx;
    
    ASSERT(block_read_start_idx < 139 && block_read_end_idx < 139);	    
    
    int32_t indirect_block_table;
    uint32_t block_idx;
    
    if(read_blocks == 0) //没有增量 则就在原扇区读
    {
    	ASSERT(block_read_end_idx == block_read_start_idx);
    	if(block_read_end_idx < 12)		//在直接块中
    	{
    	    block_idx = block_read_end_idx;
    	    all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    	}
    	else
    	{
    	    indirect_block_table = file->fd_inode->i_sectors[12];
    	    ide_read(cur_part->my_disk,indirect_block_table,all_blocks + 12,1);
    	}
    }
    else	//有增量
    {
    	if(block_read_end_idx < 12)		//在直接块中
    	{
    	    block_idx = block_read_start_idx;
    	    while(block_idx <= block_read_end_idx)
    	    {
    	    	all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    	    	++block_idx;
    	    }
    	}
    	else if(block_read_start_idx < 12 && block_read_end_idx >= 12) //越界
    	{
    	    block_idx = block_read_start_idx;
    	    while(block_idx < 12)
    	    {
    	    	all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    	    	++block_idx;
    	    }
    	    indirect_block_table = file->fd_inode->i_sectors[12];
    	    ide_read(cur_part->my_disk,indirect_block_table,all_blocks + 12,1);
    	}
    	else
    	{
    	    ASSERT(file->fd_inode->i_sectors[12] != 0); 
    	    indirect_block_table = file->fd_inode->i_sectors[12];
    	    ide_read(cur_part->my_disk,indirect_block_table,all_blocks + 12,1);
    	}
    }
    //开始读数据
    uint32_t sec_idx,sec_lba,sec_off_bytes,sec_left_bytes,chunk_size;
    uint32_t bytes_read = 0;
    
    while(bytes_read < size)
    {
    	sec_idx = file->fd_pos / BLOCK_SIZE;
    	sec_lba = all_blocks[sec_idx];
    	sec_off_bytes = file->fd_pos % BLOCK_SIZE;
    	sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
    	chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;
    	
        memset(io_buf,0,BLOCK_SIZE);
        ide_read(cur_part->my_disk,sec_lba,io_buf,1);
        memcpy(buf_dst,io_buf + sec_off_bytes,chunk_size);
        
        buf_dst += chunk_size;
        file->fd_pos += chunk_size;
        bytes_read += chunk_size;
        size_left -= chunk_size;
    }
    ASSERT(size_left == 0);
    sys_free(all_blocks);
    sys_free(io_buf);
    return bytes_read;
}

//写入文件
int32_t file_write(struct file* file,const void* buf,uint32_t count)
{
    if((file->fd_inode->i_size + count) > (BLOCK_SIZE * 140))	//超过总数字节数 则不分配
    {
    	printk("file_write: exceed max file_size 71680 bytes, write file failed\n"); //512 * 140 = 71680
    	return -1;
    }
    
    uint8_t* io_buf = (uint8_t*)sys_malloc(512);
    if(io_buf == NULL)
    {
    	printk("file_write: sys_malloc for io_buf failed\n");
    	return -1;
    }
    
    uint32_t* all_blocks = (uint32_t*)sys_malloc(BLOCK_SIZE + 48);	//48是直接块访问 512是间接块访问
    if(all_blocks == NULL)
    {
    	printk("file_write: sys_malloc for all blocks failed\n");
    	return -1;
    }
    
    const uint8_t* src = buf;						//指向代写入数据
    uint32_t bytes_written = 0;					//已经写入的数据
    uint32_t size_left     = count;					//剩下没写的字节
    int32_t  block_lba     = -1;					//块地址
    uint32_t block_bitmap_idx = 0;
    
    uint32_t sec_idx;							// 索引扇区
    uint32_t sec_lba;							// 扇区地址
    uint32_t sec_off_bytes;						// 扇区内字节偏移量
    uint32_t sec_left_bytes;						// 扇区内剩余字节数
    uint32_t chunk_size;						// 一次写入硬盘的数据量
    int32_t  indirect_block_table;					// 一级间接块地址
    uint32_t block_idx;						// 块索引
    
    //看看是否第一次写入数据 是的话先分配一个扇区
    if(file->fd_inode->i_sectors[0] == 0)
    {
    	block_lba = block_bitmap_alloc(cur_part);
    	if(block_lba == -1)	//分配失败
    	{
    	    printk("file_write: block_bitmap_alloc failed\n");
    	    sys_free(all_blocks);
    	    sys_free(io_buf);
    	    return -1;
    	}
    	file->fd_inode->i_sectors[0] = block_lba;
    	
    	block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    	ASSERT(block_bitmap_idx != 0);
    	bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    }
    
    
    uint32_t file_has_used_blocks = file->fd_inode->i_size / BLOCK_SIZE + 1;	//不满一扇区的也要当一扇区
    uint32_t file_will_use_blocks = (file->fd_inode->i_size + count) / BLOCK_SIZE + 1;
    ASSERT(file_will_use_blocks <= 140);
    
    uint32_t add_blocks = file_will_use_blocks - file_has_used_blocks;//求增量扇区 为0表示不需要
    
    //在原先分配的扇区继续写内容
    if(add_blocks == 0)
    {
    	//原先分配的12个 直接使用的扇区
    	if(file_has_used_blocks <= 12)
    	{
    	    block_idx = file_has_used_blocks - 1;//从最后一个扇区开始
    	    all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    	}
    	else
    	{
    	    ASSERT(file->fd_inode->i_sectors[12] != 0);
    	    indirect_block_table = file->fd_inode->i_sectors[12];
    	    ide_read(cur_part->my_disk,indirect_block_table,all_blocks + 12,1);//间接块内容读进去了
    	}
    }
    else
    {
    	if(file_will_use_blocks <= 12)
    	{
    	    block_idx = file_has_used_blocks - 1;		  //继续写最后一块没有写完的空间
    	    ASSERT(file->fd_inode->i_sectors[block_idx] != 0);
    	    all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    	    
    	    block_idx = file_has_used_blocks;			  //即将要新分配的扇区位置
    	    while(block_idx < file_will_use_blocks)
    	    {
    	    	block_lba = block_bitmap_alloc(cur_part);
    	    	if(block_lba == -1)
    	    	{
    	    	    printk("file_write: block_bitmap_alloc for situation 1 failed\n");
    	    	    sys_free(all_blocks);
    	    	    sys_free(io_buf);
    	    	    return -1;
    	    	}
    	        ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
    	        file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
    	        block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    	        bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    	    	++block_idx;
    	    }
    	}
    	else if(file_has_used_blocks <= 12 && file_will_use_blocks > 12) //直接块可能分配完了 要分配间接块了
    	{
    	    //把即将要写入的空闲区放到all_block[block_idx]
    	    block_idx = file_has_used_blocks - 1;
    	    all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    	    
    	    block_lba = block_bitmap_alloc(cur_part);
    	    if(block_lba == -1)
    	    {
    	    	printk("file_write: block_bitmap_alloc for situation 2 failed\n");
    	    	sys_free(all_blocks);
    	    	sys_free(io_buf);
    	    	return -1;
    	    }
    	    
    	    ASSERT(file->fd_inode->i_sectors[12] == 0);
    	    indirect_block_table = file->fd_inode->i_sectors[12] = block_lba; //先把一级间接表分配了
    	    
    	    /*这里书上没有写 但是肯定是必须要有同步的 不然是肯定错误的*/
    	    block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    	    bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    	    
    	    block_idx = file_has_used_blocks;
    	    
    	    while(block_idx < file_will_use_blocks)
    	    {
    	    	block_lba = block_bitmap_alloc(cur_part);
    	    	if(block_lba == -1)
    	    	{
    	    	    printk("file_write: block_bitmap_alloc for situation 2 failed\n");
    	    	    sys_free(all_blocks);
    	    	    sys_free(io_buf);
    	    	    return -1;
    	    	}
    	    	
    	    	if(block_idx < 12)
    	    	{
    	    	    ASSERT(file->fd_inode->i_sectors[block_lba] == 0);
    	    	    file->fd_inode->i_sectors[block_lba] = all_blocks[block_idx] = block_lba;
    	    	}
    	    	else	all_blocks[block_idx] = block_lba;
    	    	
    	    	block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    	    	bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    	    	++block_idx;
    	    }
    	    ide_write(cur_part->my_disk,indirect_block_table,all_blocks + 12,1);
    	}
    	else if(file_has_used_blocks > 12)
    	{
    	    ASSERT(file->fd_inode->i_sectors[12] != 0);
    	    indirect_block_table = file->fd_inode->i_sectors[12];
    	    
    	    block_idx = file_has_used_blocks;
    	    
    	    while(block_idx < file_will_use_blocks)
    	    {
    	        block_lba = block_bitmap_alloc(cur_part);
    	        if(block_lba == -1)
    	        {
    	            printk("file_write: block_bitmap_alloc for situation 3 failed\n");
    	            sys_free(all_blocks);
    	    	    sys_free(io_buf);
    	    	    return -1;
    	        }
    	        
    	        all_blocks[block_idx] = block_lba;
    	    	
    	    	block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    	    	bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    	    	++block_idx;
    	    }
    	    ide_write(cur_part->my_disk,indirect_block_table,all_blocks + 12,1);
    	}
    }
    
    bool first_write_block = true;              // 有剩余空间块的标识 就表示没处理第一个有空余块的标识
    file->fd_pos = file->fd_inode->i_size - 1;  // 指针放到文件大小的最后一个
    while(bytes_written < count)
    {
        memset(io_buf,0,BLOCK_SIZE);
        sec_idx = file->fd_inode->i_size / BLOCK_SIZE; //扇区起始
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes  = file->fd_inode->i_size % BLOCK_SIZE; //扇区中的偏移位置
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
        
        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;
        if(first_write_block)
        {
            ide_read(cur_part->my_disk,sec_lba,io_buf,1);
            first_write_block = false;
        }
        memcpy(io_buf + sec_off_bytes,src,chunk_size);
        ide_write(cur_part->my_disk,sec_lba,io_buf,1);
        src += chunk_size;
        file->fd_inode->i_size += chunk_size;
        file->fd_pos += chunk_size;
        bytes_written += chunk_size;
        size_left -= chunk_size;
    }
    inode_sync(cur_part,file->fd_inode,io_buf);
    sys_free(all_blocks);
    sys_free(io_buf);
    return bytes_written;
}
