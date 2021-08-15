#ifndef __FS_FS_H
#define __FS_FS_H
#include "ide.h"

#define MAX_FILES_PER_PART     4096  //每个扇区最大支持文件数
#define BITS_PER_SECTOR	4096  //每扇区的位数 512字节 * 8
#define SECTOR_SIZE		512   //每扇区的字节数 512字节
#define BLOCK_SIZE	SECTOR_SIZE   //块字节大小 我们这里一块 == 一扇区
#define MAX_PATH_LEN           512   //最大路径长度

extern struct partition* cur_part;

enum file_types 
{
    FT_UNKNOWN,	//未知文件类型
    FT_REGULAR,	//普通文件类型
    FT_DIRECTORY	//目录文件类型
};

enum oflags
{
    O_RDONLY,		//只读属性
    O_WRONLY,		//只写属性
    O_RDWR,		//可读写
    O_CREAT = 4	//创建
};

enum whence
{
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};

struct path_search_record
{
    char searched_path[MAX_PATH_LEN]; //父路径
    struct dir* parent_dir;	       //文件直接的父路径 或者说上一级的路径
    enum file_types file_type;        //普通文件 目录 或者未知类型
};

struct stat
{
    uint32_t st_ino;		       //inode编号
    uint32_t st_size;		       //尺寸
    enum file_types st_filetype;      //文件类型
};

void partition_format(struct disk* hd,struct partition* part);
bool mount_partition(struct list_elem* pelem,int arg);
void filesys_init(void);
char* path_parse(char* pathname,char* name_store);
int32_t path_depth_cnt(char* pathname);
int search_file(const char* pathname,struct path_search_record* searched_record);
int32_t sys_open(const char* pathname,uint8_t flags);
uint32_t fd_local2global(uint32_t local_fd);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd,const void* buf,uint32_t count);
int32_t sys_read(int32_t fd,void* buf,uint32_t count);
int32_t sys_lseek(int32_t fd,int32_t offset,uint8_t whence);
int32_t sys_unlink(const char* pathname);
int32_t sys_mkdir(const char* pathname);
struct dir* sys_opendir(const char* name);
int32_t sys_closedir(struct dir* dir);
struct dir_entry* sys_readdir(struct dir* dir);
void sys_rewinddir(struct dir* dir);
int32_t sys_rmdir(const char* pathname);
uint32_t get_parent_dir_inode_nr(uint32_t child_inode_nr,void* io_buf);
int get_child_dir_name(uint32_t p_inode_nr,uint32_t c_inode_nr,char* path,void* io_buf);
char* sys_getcwd(char* buf,uint32_t size);
int32_t sys_chdir(const char* path);
int32_t sys_stat(const char* path,struct stat* buf);

#endif
