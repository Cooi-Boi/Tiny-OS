#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "list.h"
#include "global.h"
#include "stdint.h"
#include "ide.h"

struct inode
{
    uint32_t i_no;	     		//inode 编号
    uint32_t i_size;	     		//文件大小 或者 目录项总大小 inode不管是什么文件的
    
    uint32_t i_open_cnts;   		//记录此文件被打开的次数
    bool write_deny;	     		//写文件不能并行
    
    uint32_t i_sectors[13]; 		//这里只实现了一级简介块 12为一级简介块指针 0-11直接是inode编号
    struct list_elem inode_tag;	//从硬盘读取速率太慢 此list做缓冲用 当第二次使用时如果list中有
    					//直接通过list_elem得到inode而不用再读取硬盘
};

struct inode_position
{
    bool two_sec;			//是否inode存储位置在两个扇区间
    uint32_t sec_lba;			//inode所在的扇区号
    uint32_t off_size;			//在所存储的扇区的偏移位置
};

void inode_locate(struct partition* part,uint32_t inode_no,struct inode_position* inode_pos);
void inode_sync(struct partition* part,struct inode* inode,void* io_buf);
struct inode* inode_open(struct partition* part,uint32_t inode_no);
void inode_close(struct inode* inode);
void inode_init(uint32_t inode_no,struct inode* new_inode);
void inode_delete(struct partition* part,uint32_t inode_no,void* io_buf);
void inode_release(struct partition* part,uint32_t inode_no);

#endif
