#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include "stdint.h"
typedef void* intr_handler;

void idt_init(void);
void register_handler(uint8_t vec_no,intr_handler function);

enum intr_status
{
    INTR_ON,
    INTR_OFF
};

enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_get_status(void);

#endif

