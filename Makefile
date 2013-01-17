#
# Makefile for busy-wait IO tests
#
XCC     = gcc
AS	= as
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

all: libbwio.a main.s main.elf

main.s: main.c
	$(XCC) -S $(CFLAGS) main.c

main.o: main.s
	$(AS) $(ASFLAGS) -o main.o main.s

main.elf: main.o
	$(LD) $(LDFLAGS) -o $@ main.o -lbwio -lgcc

libbwio.a:
	$(MAKE) -C ./io all

clean:
	-rm -f *.elf *.s *.o main.map
	$(MAKE) -C ./io clean

install:
	cp *.elf ~/cs452/tftp
	chmod a+r ~/cs452/tftp/*
