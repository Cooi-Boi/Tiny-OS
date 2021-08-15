#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H

struct super_block
{
    uint32_t magic;			//表示文件系统类型
    
    uint32_t sec_cnt;			//扇区数量
    uint32_t inode_cnt;		//分区中的inode数量
    uint32_t part_lba_base;    	//分区lba起始位置
    
    uint32_t block_bitmap_lba;	//块位图本身起始扇区lba地址
    uint32_t block_bitmap_sects;	//块位图所占的扇区数
    
    uint32_t inode_bitmap_lba;	//inode位图本身起始扇区lba地址
    uint32_t inode_bitmap_sects;	//inode位图所占的扇区数
    
    uint32_t inode_table_lba;		//inode数组本身起始扇区lba地址
    uint32_t inode_table_sects;	//inode数组所占扇区数
    
    uint32_t data_start_lba;		//存放数据区开始的第一个扇区号
    uint32_t root_inode_no;		//根目录所在的inode号
    uint32_t dir_entry_size;		//目录项大小
    
    uint8_t pad[460];			//13 * 4 + 460 = 512 为了后面位图起始位置一些列方便
}; __attribute__ ((packed));

#endif
