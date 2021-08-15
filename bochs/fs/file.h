#ifndef __FS_FILE_H
#define __FS_FILE_H

#include "inode.h"
#include "stdint.h"
#include "dir.h"

#define MAX_FILE_OPEN 32	//系统最大打开文件数
struct file
{
    uint32_t fd_pos;		//文件偏移位置
    uint32_t fd_flag;		
    struct inode* fd_inode;    //指向下一级的inode指针
};

enum std_fd
{
    stdin_no,			//标准输入
    stdout_no,			//标准输入
    stderr_no			//标准错误
};

enum bitmap_type
{
    INODE_BITMAP,		//inode位图
    BLOCK_BITMAP		//块位图
};

extern struct file file_table[MAX_FILE_OPEN];

uint32_t get_free_slot_in_global(void);
uint32_t pcb_fd_install(int32_t global_fd_idx);
int32_t inode_bitmap_alloc(struct partition* part);
int32_t block_bitmap_alloc(struct partition* part);
void bitmap_sync(struct partition* part,uint32_t bit_idx,uint8_t btmp);
int32_t file_create(struct dir* parent_dir,char* filename,uint8_t flag);
int32_t file_open(uint32_t inode_no,uint8_t flag);
int32_t file_close(struct file* file);
int32_t file_write(struct file* file,const void* buf,uint32_t count);
int32_t file_read(struct file* file,void* buf,uint32_t count);

#endif




