#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "../device/timer.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../userprog/tss.h"
#include "../userprog/syscall-init.h"
#include "../device/ide.h"
#include "../fs/fs.h"

/*负责初始化所有模块 */
void init_all() {
   put_str("init_all\n");
   idt_init();	     // 初始化中断
   mem_init();
   timer_init();
   thread_init();
   console_init();
   keyboard_init();
   tss_init();
   syscall_init();
   ide_init();
   filesys_init();
}
