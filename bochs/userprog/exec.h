#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H

#include "stdint.h"

typedef uint32_t Elf32_Word,Elf32_Addr,Elf32_Off;
typedef uint16_t Elf32_Half;

//Elf32位的头
struct Elf32_Ehdr
{
    unsigned char e_ident[16];
    Elf32_Half    e_type;   	//目标文件类型
    Elf32_Half    e_machine;	//体系结构 EM_386 3
    Elf32_Word    e_version;   //版本 1
    Elf32_Addr    e_entry;     //入口地址
    Elf32_Off     e_phoff;     //Programme Header Off
    Elf32_Off     e_shoff;     //Segment   Header Off
    Elf32_Word    e_flags;     //0x0
    Elf32_Half    e_ehsize;    //Elfsize
    Elf32_Half    e_phentsize; //Programme Header Size
    Elf32_Half    e_phnum;     //num of Programme Header
    Elf32_Half    e_shentsize; //Segment   Header Size
    Elf32_Half    e_shnum;     //Segment   Header Number
    Elf32_Half    e_shstmdx;
};

//程序头表
struct Elf32_Phdr
{
    Elf32_Word    p_type;
    Elf32_Off     p_offset;
    Elf32_Addr    p_vaddr;
    Elf32_Addr    p_paddr;
    Elf32_Word    p_filesz;
    Elf32_Word    p_memsz;
    Elf32_Word    p_flags;
    Elf32_Word    p_align;
};

//段类型
enum segment_type
{
    PT_NULL,
    PT_LOAD,    //可加载程序段
    PT_DYNAMIC, //动态加载信息
    PT_INTERP,  //动态加载器名称
    PT_NOTE,    //辅助信息
    PT_SHLIB,   //保留
    PT_PHDR     //程序头表
};

bool segment_load(int32_t fd,uint32_t offset,uint32_t filesz,uint32_t vaddr);
int32_t load(const char* pathname);
int32_t sys_execv(const char* path,const char* argv[]);

#endif
