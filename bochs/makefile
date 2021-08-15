BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/ 
ASFLAGS = -f elf
CFLAGS = -Wall -m32 -fno-stack-protector $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS =  -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
      $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/switch.o \
      $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/memory.o \
      $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o \
      $(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o $(BUILD_DIR)/keyboard.o \
      $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o \
      $(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/stdio.o \
      $(BUILD_DIR)/stdio-kernel.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/fs.o $(BUILD_DIR)/inode.o \
      $(BUILD_DIR)/file.o $(BUILD_DIR)/dir.o $(BUILD_DIR)/fork.o $(BUILD_DIR)/shell.o \
      $(BUILD_DIR)/buildin_cmd.o $(BUILD_DIR)/exec.o
      
##############     c代码编译     ###############
$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h \
        lib/stdint.h kernel/init.h lib/string.h kernel/memory.h \
        thread/thread.h kernel/interrupt.h device/console.h \
        device/keyboard.h device/ioqueue.h userprog/process.h \
        lib/user/syscall.h userprog/syscall-init.h lib/stdio.h \
        lib/kernel/stdio-kernel.h fs/file.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h \
        lib/stdint.h kernel/interrupt.h device/timer.h kernel/memory.h \
        thread/thread.h device/console.h device/keyboard.h userprog/tss.h \
        userprog/syscall-init.h device/ide.h fs/fs.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h \
        lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h \
        kernel/kernel.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/kernel/io.h lib/kernel/print.h \
        kernel/interrupt.h thread/thread.h kernel/debug.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h \
        lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/string.o: lib/string.c lib/string.h \
	kernel/debug.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h \
	lib/stdint.h lib/kernel/bitmap.h kernel/debug.h lib/string.h kernel/global.h \
	thread/sync.h thread/thread.h lib/kernel/list.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h kernel/global.h \
	lib/string.h kernel/interrupt.h lib/kernel/print.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h \
	lib/stdint.h lib/string.h kernel/global.h kernel/memory.h \
	kernel/debug.h kernel/interrupt.h lib/kernel/print.h \
	userprog/process.h thread/sync.h lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/list.o: lib/kernel/list.c lib/kernel/list.h \
	kernel/interrupt.h lib/stdint.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/sync.o: thread/sync.c thread/sync.h \
	lib/stdint.h thread/thread.h kernel/debug.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/console.o: device/console.c device/console.h \
	lib/kernel/print.h thread/sync.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/keyboard.o: device/keyboard.c device/keyboard.h \
	lib/kernel/print.h lib/kernel/io.h kernel/interrupt.h \
	kernel/global.h lib/stdint.h device/ioqueue.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/ioqueue.o: device/ioqueue.c device/ioqueue.h \
	kernel/interrupt.h kernel/global.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/tss.o: userprog/tss.c userprog/tss.h \
	kernel/global.h thread/thread.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/process.o: userprog/process.c userprog/process.h \
	lib/string.h kernel/global.h kernel/memory.h lib/kernel/print.h \
	thread/thread.h kernel/interrupt.h kernel/debug.h device/console.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c userprog/syscall-init.h \
	lib/user/syscall.h lib/stdint.h lib/kernel/print.h kernel/interrupt.h thread/thread.h \
	kernel/memory.h fs/file.h userprog/fork.h lib/kernel/stdio-kernel.h userprog/exec.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/syscall.o: lib/user/syscall.c lib/user/syscall.h fs/file.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio.o: lib/stdio.c lib/stdio.h lib/stdint.h lib/string.h lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/stdio-kernel.o: lib/kernel/stdio-kernel.c lib/kernel/stdio-kernel.h \
	lib/stdio.h device/console.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/ide.o: device/ide.c device/ide.h lib/stdint.h kernel/debug.h \
	lib/kernel/stdio-kernel.h lib/stdio.h kernel/global.h thread/sync.h \
	lib/kernel/io.h device/timer.h kernel/interrupt.h lib/kernel/list.h fs/fs.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/fs.o: fs/fs.c fs/fs.h lib/stdint.h kernel/global.h device/ide.h fs/inode.h fs/dir.h \
	fs/super_block.h lib/kernel/stdio-kernel.h lib/string.h kernel/debug.h lib/kernel/list.h \
	fs/file.h thread/thread.h device/ioqueue.h device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/inode.o: fs/inode.c fs/inode.h device/ide.h kernel/debug.h thread/thread.h \
	kernel/memory.h lib/string.h lib/kernel/list.h kernel/interrupt.h lib/kernel/bitmap.h \
	fs/file.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/file.o: fs/file.c fs/file.h lib/kernel/stdio-kernel.h thread/thread.h device/ide.h \
	fs/file.h kernel/global.h kernel/interrupt.h device/console.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/dir.o: fs/dir.c fs/dir.h device/ide.h fs/fs.h fs/inode.h kernel/memory.h lib/string.h lib/stdint.h \
	lib/kernel/stdio-kernel.h kernel/debug.h fs/file.h kernel/memory.h lib/string.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/fork.o: userprog/fork.c userprog/fork.h kernel/global.h lib/stdint.h lib/string.h \
	kernel/memory.h kernel/interrupt.h thread/sync.h thread/thread.h  kernel/debug.h userprog/process.h \
	lib/kernel/stdio-kernel.h fs/file.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/shell.o: shell/shell.c shell/shell.h kernel/global.h lib/stdint.h lib/string.h \
	lib/user/syscall.h lib/stdio.h fs/file.h kernel/debug.h shell/buildin_cmd.h fs/fs.h \
	userprog/exec.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/buildin_cmd.o: shell/buildin_cmd.c shell/buildin_cmd.h fs/file.h fs/fs.h kernel/debug.h \
	lib/string.h lib/user/syscall.h fs/dir.h fs/fs.h lib/stdio.h shell/shell.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/exec.o: userprog/exec.c userprog/exec.h kernel/global.h kernel/memory.h fs/fs.h \
	fs/file.h lib/kernel/stdio-kernel.h kernel/interrupt.h lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@

##############    汇编代码编译    ###############
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o: thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all

mk_dir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

hd:
	dd if=$(BUILD_DIR)/kernel.bin \
           of=/home/cooiboi/bochs/hd60M.img \
           bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f  ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd
