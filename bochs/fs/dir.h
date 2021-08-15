#ifndef __FS_DIR_H
#define __FS_DIR_H

#define MAX_FILE_NAME_LEN	16 //最长16个字
#include "ide.h"
#include "fs.h"

struct dir			//在内存中用的目录结构
{
    struct inode* inode;	//指向已经打开的inode指针
    uint32_t dir_pos;		//目录偏移位置
    uint8_t  dir_buf[512];	//目录的数据缓冲
};

struct dir_entry			//目录项
{
    char filename[MAX_FILE_NAME_LEN];	//16个字的名字
    uint32_t i_no;			//inode编号
    enum file_types f_type;		//文件类型 由此看是目录还是文件类型
};

extern struct dir root_dir;

void open_root_dir(struct partition* part);
struct dir* dir_open(struct partition* part,uint32_t inode_no);
bool search_dir_entry(struct partition* part,struct dir* pdir,const char* name,struct dir_entry* dir_e);
void dir_close(struct dir* dir);
void create_dir_entry(char* filename,uint32_t inode_no,uint8_t file_type,struct dir_entry* p_de);
bool sync_dir_entry(struct dir* parent_dir,struct dir_entry* p_de,void* io_buf);
bool delete_dir_entry(struct partition* part,struct dir* pdir,uint32_t inode_no,void* io_buf);
struct dir_entry* dir_read(struct dir* dir);
bool dir_is_empty(struct dir* dir);
int32_t dir_remove(struct dir* parent_dir,struct dir* child_dir);

#endif
