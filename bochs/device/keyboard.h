#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H

extern struct ioqueue ioqueue;
void intr_keyboard_handler(void);
void keyboard_init(void);

#endif
