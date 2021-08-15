#include "dir.h"
#include "ide.h"
#include "fs.h"
#include "inode.h"
#include "memory.h"
#include "string.h"
#include "stdint.h"
#include "stdio-kernel.h"
#include "debug.h"
#include "file.h"

struct dir root_dir;

//打开根目录
void open_root_dir(struct partition* part)
{
    root_dir.inode = inode_open(part,part->sb->root_inode_no);
    root_dir.dir_pos = 0;
}

//打开指定inode结点中的目录 返回目录指针
struct dir* dir_open(struct partition* part,uint32_t inode_no)
{
    struct dir* pdir = (struct dir*)sys_malloc(sizeof(struct dir));
    pdir->inode = inode_open(part,inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

//在part分区找名字为name的文件或者目录
//找到后返回true 并且把目录项存放到dir_e中 以后可能解析目录的时候通过递归来实现
bool search_dir_entry(struct partition* part,struct dir* pdir,const char* name,struct dir_entry* dir_e)
{
     uint32_t block_cnt = 140; //inode中12个直接指 + 1个128间接 = 140 128 = 512/4
     uint32_t* all_blocks = (uint32_t*)sys_malloc(block_cnt*4);
     if(all_blocks == NULL)
     {
         printk("search_dir_entry: sys_malloc for all_blocks failed\n");
         return false;
     }
     
     uint32_t block_idx = 0;
     while(block_idx < 12)
     {
         all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
         ++block_idx;
     }
     
     block_idx = 0;
     
     //如果存在一级间接块表
     if(pdir->inode->i_sectors[12] != 0)
         ide_read(part->my_disk,pdir->inode->i_sectors[12],all_blocks,1);
     
     //每次只分配一个扇区大小 之后我们把目录项不会再出现放到两个扇区之间的情况
     uint8_t* buf = (uint8_t*)sys_malloc(SECTOR_SIZE);
     struct dir_entry* p_de = (struct dir_entry*)buf;
     
     uint32_t dir_entry_size = part->sb->dir_entry_size;
     uint32_t dir_entry_cnt  = SECTOR_SIZE / dir_entry_size;
     
     while(block_idx < block_cnt)
     {
     	 //说明此处没有目录文件 继续遍历
         if(all_blocks[block_idx] == 0)
         {
             ++block_idx;
             continue;
         }
         //把其内容读出来
         ide_read(part->my_disk,all_blocks[block_idx],buf,1);
         
         uint32_t dir_entry_idx = 0;
         while(dir_entry_idx < dir_entry_cnt)
         {
             if(!strcmp(p_de->filename,name))
             {
                 memcpy(dir_e,p_de,dir_entry_size);	//把目录项内容复制到dir_e指针区
                 sys_free(buf);
                 sys_free(all_blocks);
                 return true;
             }
             ++p_de;
             ++dir_entry_idx;
         }
         ++block_idx;
         p_de = (struct dir_entry*)buf;		//赋值还原回去
	 memset(buf,0,SECTOR_SIZE);         		//全部设置为0 初始化buf
     }
     sys_free(buf);
     sys_free(all_blocks);
     return false;
}

//关闭目录
void dir_close(struct dir* dir)
{
    if(dir == &root_dir)
        return;
    inode_close(dir->inode);
    sys_free(dir);
}

//在内存中初始化目录项
void create_dir_entry(char* filename,uint32_t inode_no,uint8_t file_type,struct dir_entry* p_de)
{
    ASSERT(strlen(filename) <= MAX_FILE_NAME_LEN);
    memcpy(p_de->filename,filename,strlen(filename));
    p_de->i_no = inode_no;
    p_de->f_type = file_type;
}

//将目录项p_de 写入父目录parent_dir io_buf由主调函数提供
bool sync_dir_entry(struct dir* parent_dir,struct dir_entry* p_de,void* io_buf)
{
    struct inode* dir_inode = parent_dir->inode;
    uint32_t dir_size = dir_inode->i_size;
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    
    ASSERT(dir_size % dir_entry_size == 0);
    
    uint32_t dir_entrys_per_sec = (512 / dir_entry_size);	//得到每扇区最多多少个目录项
    int32_t block_lba = -1;					
    uint8_t block_idx = 0;
    uint32_t all_blocks[140] = {0};				//局部变量 所有块
    
    //先看直接块 后面再看简介块
    while(block_idx < 12)
    {
    	all_blocks[block_idx] = dir_inode->i_sectors[block_idx];	//直接块读取
    	block_idx++;
    }
    
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;		//遍历目录项的临时指针
    int32_t block_bitmap_idx = -1;					//位图idx
    
    block_idx = 0;
    while(block_idx < 140)
    {
    	block_bitmap_idx = -1;
    	if(all_blocks[block_idx] == 0)				//此块还没有分配
    	{
    	    block_lba = block_bitmap_alloc(cur_part);
    	    if(block_lba == -1)
    	    {
    	        printk("alloc block bitmap for sync_dir_entry failed\n");
    	        return false;
    	    }
    	    
    	    block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;//由分配扇区号 - 起始扇区号 = 位图的相对偏移
    	    bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);        //同步到硬盘
    	    
    	    block_bitmap_idx = -1;
    	    if(block_idx < 12)		//直接块
    	    	dir_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
    	    else if(block_idx == 12)	//一级间接表 间接表
    	    {
    	    	dir_inode->i_sectors[block_idx] = block_lba;
    	    	block_lba = -1;
    	    	block_lba = block_bitmap_alloc(cur_part);	//再给到一个块 给简介块第0个赋值
    	        if(block_lba == -1)	//分配失败则需要回退之前的分配
    	        {
    	            block_bitmap_idx = dir_inode->i_sectors[12] - cur_part->sb->data_start_lba;
    	            bitmap_set(&cur_part->block_bitmap,block_bitmap_idx,0);
    	            dir_inode->i_sectors[12] = 0;
    	            printk("alloc block bitmap for sync_dir_entry failed\n");
    	            return false;
    	        }
    	        block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
    	        ASSERT(block_bitmap_idx != -1);
    	        bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
    	        all_blocks[12] = block_lba;
    	        ide_write(cur_part->my_disk,dir_inode->i_sectors[12],all_blocks + 12,1);	//一级间接块复制到硬盘中
    	    }
    	    else	//已经有一级间接表了 
    	    {
    	    	all_blocks[block_idx] = block_lba;
    	    	ide_write(cur_part->my_disk,dir_inode->i_sectors[12],all_blocks + 12,1);	//一级间接表复制到硬盘中
    	    }
    	    
    	    memset(io_buf,0,512);
    	    memcpy(io_buf,p_de,dir_entry_size);
    	    ide_write(cur_part->my_disk,all_blocks[block_idx],io_buf,1);			//把目录项放到新分配的区块
    	    dir_inode->i_size += dir_entry_size;
    	    return true;
    	}
	
	//此块已经被分配 寻找空余空间 
    	ide_read(cur_part->my_disk,all_blocks[block_idx],io_buf,1);
    	uint8_t dir_entry_idx = 0;
    	while(dir_entry_idx < dir_entrys_per_sec)	//由于我们存放目录项不再出现存放于两个扇区间的情况 即<dir_entrys_per_sec即可
    	{
    	    if((dir_e + dir_entry_idx)->f_type == FT_UNKNOWN)	//这个位置还没有存放目录项
    	    {
    	    	memcpy(dir_e + dir_entry_idx,p_de,dir_entry_size);
    	    	ide_write(cur_part->my_disk,all_blocks[block_idx],io_buf,1);	//写回去
    	    	dir_inode->i_size += dir_entry_size;				//文件大小增加
    	    	return true;
    	    }
    	    ++dir_entry_idx;
    	}
    	++block_idx;	//遍历完140个块
    }
    printk("directory is full!\n");
    return false;
}

//把pdir中编号为inode_no目录项删除
bool delete_dir_entry(struct partition* part,struct dir* pdir,uint32_t inode_no,void* io_buf)
{
    struct inode* dir_inode = pdir->inode;
    uint32_t block_idx = 0,all_blocks[140] = {0};
    while(block_idx < 12)
    {
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
        ++block_idx;
    }
    if(dir_inode->i_sectors[12])
        ide_read(part->my_disk,dir_inode->i_sectors[12],all_blocks + 12,1);
        
    //目录项存储不像inode那样存在夹在两个扇区间的情况    
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entrys_per_sec = (SECTOR_SIZE / dir_entry_size);
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;
    struct dir_entry* dir_entry_found = NULL;
    uint8_t dir_entry_idx,dir_entry_cnt;
    bool is_dir_first_block = false;		//目录的第一个块
    
    block_idx = 0;
    while(block_idx < 140)
    {
        is_dir_first_block = false;
        if(all_blocks[block_idx] == 0)
        {
            ++block_idx;
            continue;
        }
        
        dir_entry_idx = dir_entry_cnt = 0;
        memset(io_buf,0,SECTOR_SIZE);
        ide_read(part->my_disk,all_blocks[block_idx],io_buf,1);
        
        //挨个挨个遍历寻找
        while(dir_entry_idx < dir_entrys_per_sec)
        {
            if((dir_e + dir_entry_idx)->f_type != FT_UNKNOWN)
            {
            	//找到根目录所在扇区了 说明不可回收此扇区
                if(!strcmp((dir_e + dir_entry_idx)->filename,"."))
                {
                    is_dir_first_block = true;
                }
                else if(strcmp((dir_e + dir_entry_idx)->filename,".") && strcmp((dir_e + dir_entry_idx)->filename,".."))
                {
                    dir_entry_cnt++; //判断是否回收此扇区
                    if((dir_e + dir_entry_idx)->i_no == inode_no)
                    {
                        ASSERT(dir_entry_found == NULL);
                        dir_entry_found = dir_e + dir_entry_idx;
                        //继续遍历 统计共有多少目录项
                    }
                }
            }
            ++dir_entry_idx;
        }
        
        //这个扇区没找到 到一下个扇区继续找
        if(dir_entry_found == NULL)
        {
            ++block_idx;
            continue;
        }
        
        ASSERT(dir_entry_cnt >= 1);
        
        //不包括根目录 该块且只有一个目录项 这个目录项刚好是我们需要删除的
        if(dir_entry_cnt == 1 && !is_dir_first_block)
        {
            uint32_t block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
            bitmap_set(&part->block_bitmap,block_bitmap_idx,0);
            bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);  //块同步进去
            
            if(block_idx < 12)
                dir_inode->i_sectors[block_idx]  = 0;
            else //还需要判断是否要把间接块表回收回去
            {
                uint32_t indirect_blocks = 0;
                uint32_t indirect_block_idx = 12;
                while(indirect_block_idx  < 140)
                {
                    if(all_blocks[indirect_block_idx] != 0)
                         ++indirect_blocks;
                }
                ASSERT(indirect_blocks >= 1); //至少一个
                
                if(indirect_blocks > 1) //间接表块不回收
                {
                    all_blocks[block_idx] = 0; //那个地方的块地址写成0
                    ide_write(part->my_disk,dir_inode->i_sectors[12],all_blocks+ 12,1);
                }
                else //先回收那个块表
                {
                    /*这里是我加的*/
                    all_blocks[block_idx] = 0; //那个地方的块地址写成0
                    ide_write(part->my_disk,dir_inode->i_sectors[12],all_blocks+ 12,1);
                    /*我认为这是必须加的两行*/
                    
                    block_bitmap_idx = dir_inode->i_sectors[12] - part->sb->data_start_lba;
                    bitmap_set(&part->block_bitmap,block_bitmap_idx,0);
                    bitmap_sync(cur_part,block_bitmap_idx,BLOCK_BITMAP);
                }
            }
        }
        else
        {
            memset(dir_entry_found,0,dir_entry_size);//把那里的目录项清一条即可
            ide_write(part->my_disk,all_blocks[block_idx],io_buf,1);
        }    
            
        ASSERT(dir_inode->i_size >= dir_entry_size);
        dir_inode->i_size -= dir_entry_size;  //大小减去一条目录项的
        memset(io_buf,0,SECTOR_SIZE * 2);
        inode_sync(part,dir_inode,io_buf);   //把inode写入硬盘
        return true;
    }
    return false;
}

//读取目录
struct dir_entry* dir_read(struct dir* dir)
{
    struct dir_entry* dir_e = (struct dir_entry*)dir->dir_buf; //哈哈 目录项里面自己有缓冲区
    struct inode* dir_inode = dir->inode;
    uint32_t all_blocks[140] = {0},block_cnt = 12;
    uint32_t block_idx = 0,dir_entry_idx = 0;
    
    //读取直接块
    while(block_idx < 12)
    {
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
        ++block_idx;
    }
    
    //有间接块 读入
    if(dir_inode->i_sectors[12] != 0)
    {
        ide_read(cur_part->my_disk,dir_inode->i_sectors[12],all_blocks + 12,1);
        block_cnt = 140;
    }
    block_idx = 0;
    
    uint32_t cur_dir_entry_pos = 0;		//遍历目录了
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    uint32_t dir_entrys_per_sec = SECTOR_SIZE / dir_entry_size;
    
    while(dir->dir_pos < dir_inode->i_size)
    {
        //该目录中都已经目录项已经遍历完了
        if(dir->dir_pos >= dir_inode->i_size)	return NULL;
        if(all_blocks[block_idx] == 0)
        {
            block_idx++;
            continue;
        }
        memset(dir_e,0,SECTOR_SIZE);
        ide_read(cur_part->my_disk,all_blocks[block_idx],dir_e,1);
        dir_entry_idx = 0;
        while(dir_entry_idx < dir_entrys_per_sec)
        {
            //FT_UNKNOWN是未知
            if((dir_e + dir_entry_idx)->f_type)
            {
                //原来已经遍历过了 因为需要从头开始 
                if(cur_dir_entry_pos < dir->dir_pos)
                {
                    cur_dir_entry_pos += dir_entry_size;
                    ++dir_entry_idx;
                    continue;
                }
                ASSERT(cur_dir_entry_pos == dir->dir_pos);
                dir->dir_pos += dir_entry_size;
                //返回目录地址
                return dir_e + dir_entry_idx;
            }
            ++dir_entry_idx;
        }
        ++block_idx;
    }
    return NULL;
}

bool dir_is_empty(struct dir* dir)
{
    if(!dir)
    {
        printk("dir_is_empty: dir is NULL\n");
        return false;
    }
    struct inode* dir_inode = dir->inode;
    return dir_inode->i_size == cur_part->sb->dir_entry_size * 2; //目前目录里面只有. 和 ..
}

//父目录parent_dir删除指定child_dir
int32_t dir_remove(struct dir* parent_dir,struct dir* child_dir)
{
    struct inode* child_dir_inode = child_dir->inode;
    
    //如果是空目录的话 只有第一个扇区有根目录. 和 .. 其他都是空的
    int32_t block_idx = 1;
    while(block_idx < 13)
    {
        ASSERT(child_dir_inode->i_sectors[block_idx] == 0);
        ++block_idx;
    }
    void* io_buf = sys_malloc(SECTOR_SIZE * 2);
    if(io_buf == NULL)
    {
        printk("dir_remove: malloc for io_buf failed\n");
        return -1;
    }
    
    //之前就有的 删除其目录项
    delete_dir_entry(cur_part,parent_dir,child_dir_inode->i_no,io_buf);
    inode_release(cur_part,child_dir_inode->i_no);
    sys_free(io_buf);
    return 0;
}
