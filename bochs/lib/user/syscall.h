#ifndef __LIB_USER_SCSCALL_H
#define __LIB_USER_SCSCALL_H
#include "stdint.h"
#include "../fs/file.h"

enum SYSCALL_NR
{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_READ,
    SYS_PUTCHAR,
    SYS_CLEAR,
    SYS_GETCWD,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_MKDIR,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_CHDIR,
    SYS_RMDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS,
    SYS_EXECV
};

uint32_t getpid(void);
uint32_t write(int fd,const void* buf,int count);
void* malloc(uint32_t size);
void free(void* ptr);
int32_t fork(void);
int32_t read(int32_t fd,void* buf,uint32_t count);
void putchar(const char chr);
void clear(void);
char* getcwd(char* buf,uint32_t size);
int32_t open(char* pathname,uint8_t flag);
int32_t close(int32_t fd);
int32_t lseek(int32_t fd,int32_t offset,uint8_t whence);
int32_t unlink(const char* pathname);
int32_t mkdir(const char* pathname);
struct dir* opendir(const char* name);
int32_t closedir(struct dir* dir);
int32_t rmdir(const char* pathname);
struct dir_entry* readdir(struct dir* dir);
void rewinddir(struct dir* dir);
int32_t stat(const char* path,struct stat* buf);
int32_t chdir(const char* path);
void ps(void);
int32_t execv(const char* path,const char* argv[]);

#endif
