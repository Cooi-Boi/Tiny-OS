BIN="prog_no_arg"
CFLAGS="-Wall -m32 -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers"
LIB="../lib/"
OBJS="../build/string.o ../build/syscall.o ../build/stdio.o ../build/debug.o"
DD_IN=$BIN
DD_OUT="/home/cooiboi/bochs/hd60M.img"

gcc $CFLAGS -I $LIB -o $BIN".o" $BIN".c"
ld -m elf_i386 -Ttext 0x8060000 -e main $BIN".o" $OBJS -o $BIN

if [ -f $BIN ];then
	dd if=./$DD_IN of=$DD_OUT bs=512 \
	count=30 seek=300 conv=notrunc
fi
