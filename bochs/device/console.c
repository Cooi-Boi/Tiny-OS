#include "console.h"
#include "print.h"
#include "../thread/sync.h"

struct lock console_lock;

void console_init()
{
    lock_init(&console_lock);
}

void console_acquire()
{
    lock_acquire(&console_lock);
}

void console_release()
{
    lock_release(&console_lock);
}

void console_put_str(char* str)
{
    console_acquire();
    put_str(str);
    console_release();
}

void console_put_int(uint32_t num)
{
    console_acquire();
    put_int(num);
    console_release();
}

void console_put_char(uint8_t chr)
{
    console_acquire();
    put_char(chr);
    console_release();
}
