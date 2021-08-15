#include "print.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#include "memory.h"
#include "../thread/thread.h"
#include "interrupt.h"
#include "../device/console.h"
#include "../device/ioqueue.h"
#include "../device/keyboard.h"
#include "../userprog/process.h"
#include "../lib/user/syscall.h"
#include "../userprog/syscall-init.h"
#include "../lib/stdio.h"
#include "../lib/kernel/stdio-kernel.h"
#include "../fs/fs.h"
#include "../fs/file.h"
#include "../shell/shell.h"
#include "global.h"

int main(void) 
{
    put_str("I am kernel\n");
    init_all();
    intr_enable();
    
    uint32_t file_size = 10236;
    uint32_t sec_cnt   = DIV_ROUND_UP(file_size,512);
    struct disk* sda = &channels[0].devices[0];
    void* prog_buf = sys_malloc(file_size);
    ide_read(sda,300,prog_buf,sec_cnt);
    int32_t fd = sys_open("/prog_no_arg",O_CREAT | O_RDWR);
    if(fd != -1)
    {
        if(sys_write(fd,prog_buf,file_size) == -1)
        {
            printk("file write error\n");
            while(1);
        }
    }
    sys_free(prog_buf);
    
    cls_screen();
    console_put_str("[Love 6@localhost /]$ ");
    
   
    while(1);
    return 0;
}

void init(void)
{
    uint32_t ret_pid = fork();
    if(ret_pid)
        while(1);
    else
        my_shell();
    PANIC("init: should not be here");
}
