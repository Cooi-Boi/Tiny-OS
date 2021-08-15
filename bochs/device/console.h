#ifndef __DEVICE__CONSOLE_H
#define __DEVICE__CONSOLE_H

#include "console.h"
#include "print.h"
#include "../thread/sync.h"


void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char* str);
void console_put_int(uint32_t num);
void console_put_char(uint8_t chr);

#endif
