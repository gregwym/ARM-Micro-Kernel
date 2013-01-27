#
# Makefile for ARM Micro Kernel
#
XCC     = gcc
AS	    = as
LD      = ld
CFLAGS  = -c -fPIC -Wall -I. -I./include -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L./lib

OBJECTS = main.o syscall.o syscall_handler.o asm/usertrap.o

all: main.elf

%.s: %.c
	$(XCC) -S $(CFLAGS) $<

$(OBJECTS): %.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

libbwio.a:
	$(MAKE) -C ./io all

libstdlib.a:
	$(MAKE) -C ./stdlib all

main.elf: $(OBJECTS) libbwio.a libstdlib.a
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) -lstdlib -lbwio -lgcc

clean:
	-rm -f *.elf *.s *.o main.map
	-rm asm/*.o
	$(MAKE) -C ./io clean
	$(MAKE) -C ./stdlib clean

install:
	cp *.elf ~/cs452/tftp
	chmod a+r ~/cs452/tftp/*
