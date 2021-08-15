#ifndef __DEVICE_TIME_H
#define __DEVICE_TIME_H
#include "stdint.h"
void frequency_set(uint8_t counter_port ,uint8_t counter_no,uint8_t rwl,uint8_t counter_mode,uint16_t counter_value);
void intr_timer_handler(void);
void timer_init(void);
void mtime_sleep(uint32_t m_seconds);
void ticks_to_sleep(uint32_t sleep_ticks);
#endif
