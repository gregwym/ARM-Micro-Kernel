#
# Makefile for ARM Micro Kernel
#
XCC     = gcc
AS	    = as
LD      = ld
CFLAGS  = -c -fPIC -Wall -I. -I../include -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L../lib

OBJECTS = ../main.o send-receive-reply-perf.o
LIBS = services.a task.a interrupt.a klib.a
LIBS_INCLUDE = $(patsubst %.a,-l%,$(LIBS))

all: test.elf

%.s: %.c
	$(XCC) -S $(CFLAGS) -o $@ $<

$(OBJECTS): %.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

test.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS_INCLUDE) -lgcc

clean:
	-rm -f *.elf *.s $(OBJECTS) main.map

install:
	cp *.elf ~/cs452/tftp
	chmod a+r ~/cs452/tftp/*
