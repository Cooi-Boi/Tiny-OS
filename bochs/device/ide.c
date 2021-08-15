#include "ide.h"
#include "stdint.h"
#include "debug.h"
#include "stdio-kernel.h"
#include "stdio.h"
#include "global.h"
#include "../thread/sync.h"
#include "io.h"
#include "timer.h"
#include "interrupt.h"
#include "memory.h"
#include "../fs/fs.h"

#define reg_data(channel) 	  (channel->port_base + 0)
#define reg_error(channel) 	  (channel->port_base + 1)
#define reg_sect_cnt(channel)    (channel->port_base + 2)
#define reg_lba_l(channel)	  (channel->port_base + 3)
#define reg_lba_m(channel)	  (channel->port_base + 4)
#define reg_lba_h(channel)	  (channel->port_base + 5)
#define reg_dev(channel)	  (channel->port_base + 6)
#define reg_status(channel)	  (channel->port_base + 7)
#define reg_cmd(channel)	  (reg_status(channel))
#define reg_alt_status(channel)  (channel->port_base + 0x206)
#define reg_ctl(channel)	  reg_alt_status(channel)

#define BIT_STAT_BSY	  	  0X80		//硬盘忙
#define BIT_STAT_DRDY	  	  0X40		//驱动器准备好啦
#define BIT_STAT_DRQ 	  	  0x8		//数据传输准备好了
#define BIT_DEV_MBS		  0XA0		
#define BIT_DEV_LBA		  0X40
#define BIT_DEV_DEV		  0X10

#define CMD_IDENTIFY		  0XEC		//identify指令
#define CMD_READ_SECTOR	  0X20		//读扇区指令
#define CMD_WRITE_SECTOR	  0X30		//写扇区指令

#define max_lba		  ((80*1024*1024/512) - 1) //调试用

uint8_t channel_cnt;		  //通道数
struct ide_channel channels[2];  //两个ide通道

int32_t ext_lba_base = 0;	  //记录总拓展分区lba 初始为0
uint8_t p_no = 0,l_no = 0;	  //记录硬盘主分区下标 逻辑分区下标
struct list partition_list;	  //分区队列

//选择读写的硬盘
void select_disk(struct disk* hd)
{
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if(hd->dev_no == 1)    //主盘0 从盘1
    	reg_device |= BIT_DEV_DEV;
    outb(reg_dev(hd->my_channel),reg_device);
}

//向硬盘控制器写入起始扇区和读写的扇区数
void select_sector(struct disk* hd,uint32_t lba,uint8_t sec_cnt)
{
    ASSERT(lba <= max_lba);
    struct ide_channel* channel = hd->my_channel;
    
    outb(reg_sect_cnt(channel),sec_cnt);
    outb(reg_lba_l(channel),lba);
    outb(reg_lba_m(channel),lba>>8);
    outb(reg_lba_h(channel),lba>>16);
    
    outb(reg_dev(channel),BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

//向通道channel发命令cmd
void cmd_out(struct ide_channel* channel,uint8_t cmd)
{
    channel->expecting_intr = true;
    outb(reg_cmd(channel),cmd);
}

//从扇区读数据
void read_from_sector(struct disk* hd,void* buf,uint8_t sec_cnt)
{
    uint32_t size_in_byte;
    if(sec_cnt == 0)
    	size_in_byte = 256*512;
    else
    	size_in_byte = sec_cnt*512;
    insw(reg_data(hd->my_channel),buf,size_in_byte/2);	//读如数据到buf
}

//将 sec_cnt 扇区数据写入硬盘
void write2sector(struct disk* hd,void* buf,uint8_t sec_cnt)
{
    uint32_t size_in_byte;
    if(sec_cnt == 0)	size_in_byte = 256*512;
    else	size_in_byte = sec_cnt * 512;
    outsw(reg_data(hd->my_channel),buf,size_in_byte/2);
}

//最多等待三十秒 任务应该都在30秒内完成  读取硬盘相应给了整整30秒
bool busy_wait(struct disk* hd)
{
    struct ide_channel* channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000;
    while(time_limit -= 10 >= 0)
    {
    	if(!(inb(reg_status(channel)) & BIT_STAT_BSY))
    	    return (inb(reg_status(channel)) & BIT_STAT_DRQ);
    	else  mtime_sleep(10);
    }
    return false;
}

//从硬盘读取sec_cnt扇区到buf
void ide_read(struct disk* hd,uint32_t lba,void* buf,uint32_t sec_cnt)
{
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);
    
    select_disk(hd);
    
    
    uint32_t secs_op;
    uint32_t secs_done = 0;
    while(secs_done < sec_cnt)
    {
        if((secs_done + 256) <= sec_cnt)    secs_op = 256;
        else	secs_op = sec_cnt - secs_done;
    	
        select_sector(hd,lba + secs_done, secs_op);
        cmd_out(hd->my_channel,CMD_READ_SECTOR); //执行命令
       
	
        /*在硬盘开始工作时 阻塞自己 完成读操作后唤醒自己*/
        sema_down(&hd->my_channel->disk_done);
	
        /*检测是否可读*/
        if(!busy_wait(hd))
        {
            char error[64];
            sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba);
            PANIC(error);
        }
      	read_from_sector(hd,(void*)((uint32_t)buf +secs_done * 512),secs_op);
      	secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);   
}

void ide_write(struct disk* hd,uint32_t lba,void* buf,uint32_t sec_cnt)
{
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);
    
    select_disk(hd);
    uint32_t secs_op;
    uint32_t secs_done = 0;
    
    while(secs_done < sec_cnt)
    {
        if((secs_done + 256) <= sec_cnt)    secs_op = 256;
        else	secs_op = sec_cnt - secs_done;
        
    	select_sector(hd,lba+secs_done,secs_op);
    	cmd_out(hd->my_channel,CMD_WRITE_SECTOR);
    	
    	if(!busy_wait(hd))
    	{
    	    char error[64];
    	    sprintf(error,"%s write sector %d failed!!!!!!\n",hd->name,lba);
    	    PANIC(error);
    	}
    	
    	write2sector(hd,(void*)((uint32_t)buf + secs_done * 512),secs_op);
    	
    	//硬盘响应期间阻塞
    	sema_down(&hd->my_channel->disk_done);
    	secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}

//硬盘结束任务中断程序
void intr_hd_handler(uint8_t irq_no)
{
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t ch_no = irq_no - 0x20 - 0xe;
    struct ide_channel* channel = &channels[ch_no];
    ASSERT(channel->irq_no == irq_no);
    if(channel->expecting_intr)
    {
	channel->expecting_intr = false;//结束任务了
	sema_up(&channel->disk_done);
	inb(reg_status(channel));
    }   
}

//将dst中len个相邻字节交换位置存入buf 因为读入的时候字节顺序是反的 所以我们再反一次即可
void swap_pairs_bytes(const char* dst,char* buf,uint32_t len)
{
    uint8_t idx;
    for(idx = 0;idx < len;idx += 2)
    {
    	buf[idx+1] = *(dst++);
    	buf[idx]   = *(dst++);
    }
}

void partition_scan(struct disk* hd,uint32_t ext_lba)
{
    struct boot_sector* bs = sys_malloc(sizeof(struct boot_sector));
    ide_read(hd,ext_lba,bs,1);
    uint8_t part_idx = 0;
    struct partition_table_entry* p = bs->partition_table; //p为分区表开始的位置
    while((part_idx++) < 4)
    {
    	if(p->fs_type == 0x5) //拓展分区
    	{
    	    
    	    if(ext_lba_base != 0)
    	    {
    	    	partition_scan(hd,p->start_lba + ext_lba_base);	//继续递归转到下一个逻辑分区再次得到表
    	    }
    	    else //第一次读取引导块
    	    {
    	    	ext_lba_base = p->start_lba;
    	    	partition_scan(hd,ext_lba_base);
    	    }
    	}
    	else if(p->fs_type != 0)
    	{
    	    if(ext_lba == 0)	//主分区
    	    {
    	    	hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
    	    	hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
    	    	hd->prim_parts[p_no].my_disk = hd;
    	    	list_append(&partition_list,&hd->prim_parts[p_no].part_tag);
    	    	sprintf(hd->prim_parts[p_no].name,"%s%d",hd->name,p_no+1);
    	    	p_no++;
    	    	ASSERT(p_no<4);	//0 1 2 3 最多四个
    	    }
    	    else		//其他分区
    	    {
    	    	hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
    	    	hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
    	    	hd->logic_parts[l_no].my_disk = hd;
    	    	list_append(&partition_list,&hd->logic_parts[l_no].part_tag);
    	    	sprintf(hd->logic_parts[l_no].name,"%s%d",hd->name,l_no+5); //从5开始
    	    	l_no++;
    	    	if(l_no >= 8)	return; //只支持8个
    	    }
    	}
    	++p;
    }
    sys_free(bs);
}

bool partition_info(struct list_elem* pelem,int arg)
{
    struct partition* part = elem2entry(struct partition,part_tag,pelem);
    printk("    %s  start_lba:0x%x,sec_cnt:0x%x\n",part->name,part->start_lba,part->sec_cnt);
    return false; //list_pop完
}

void identify_disk(struct disk* hd)
{
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel,CMD_IDENTIFY);
    
    if(!busy_wait(hd))
    {
    	char error[64];
    	sprintf(error,"%s identify failed!!!!!!\n");
    	PANIC(error);
    }
    read_from_sector(hd,id_info,1);//现在硬盘已经把硬盘的参数准备好了了 我们把参数读到自己的缓冲区中
    
    char buf[64] = {0};
    uint8_t sn_start = 10 * 2,sn_len = 20,md_start = 27*2,md_len = 40;
    swap_pairs_bytes(&id_info[sn_start],buf,sn_len);
    printk("    disk %s info:        SN: %s\n",hd->name,buf);
    swap_pairs_bytes(&id_info[md_start],buf,md_len);
    printk("    MODULE: %s\n",buf);
    uint32_t sectors = *(uint32_t*)&id_info[60*2];
    printk("    SECTORS: %d\n",sectors);
    printk("    CAPACITY: %dMB\n",sectors * 512 / 1024 / 1024);
} 



//初始化硬盘数据
void ide_init(void)
{
    printk("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t*)(0x475));     //获取硬盘数量
    ASSERT(hd_cnt > 0);
    channel_cnt = DIV_ROUND_UP(hd_cnt,2);      //两个硬盘一个ide 通过硬盘推ide
    ASSERT(channel_cnt > 0);
    
    struct ide_channel* channel;
    uint8_t channel_no = 0,dev_no = 0;
    list_init(&partition_list);
    
    while(channel_no < channel_cnt)
    {
    	channel = &channels[channel_no];
    	sprintf(channel->name,"ide%d",channel_no);
    	
    	switch(channel_no)
    	{
    	    case 0:
    		channel->port_base = 0x1f0;		//ide0 起始端口号0x1f0
    		channel->irq_no    = 0x2e;		//8259a 中断引脚
    		break;
    	    case 1:
    		channel->port_base = 0x170;		//ide1 起始端口号0x170
    		channel->irq_no    = 0x2f;
    		break;
    	}
	register_handler(channel->irq_no,intr_hd_handler);
    	channel->expecting_intr = false;		//不期待中断
    	lock_init(&channel->lock);
    	sema_init(&channel->disk_done,0);
    	
    	while(dev_no < 2)
    	{
    	    struct disk* hd = &channel->devices[dev_no];
    	    hd->my_channel = channel;
    	    hd->dev_no = dev_no;
    	    sprintf(hd->name,"sd%c",'a' + channel_no * 2 + dev_no);
    	    identify_disk(hd);
    	    if(dev_no != 0)
    	    	partition_scan(hd,0);
    	    p_no = 0,l_no = 0;
    	    dev_no++;
    	}
    	dev_no = 0;
    	channel_no++;
    }
    printk("\n    all partition info\n");
    list_traversal(&partition_list,partition_info,(int)NULL);
    printk("ide_init done\n");
}
