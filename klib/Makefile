#
# Makefile for stdlib library
#
XCC     = gcc
AS	= as
AR	= ar
CFLAGS  = -c -fPIC -Wall -I. -I../include -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

ARFLAGS = rcs

OBJECTS = irqio.o assert.o string.o bwio.o heap.o buffer.o linkedlist.o

all: klib.a

%.s: %.c
	$(XCC) -S $(CFLAGS) -o $@ $<

$(OBJECTS): %.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

klib.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	cp *.a ../lib/libklib.a

clean:
	-rm -f *.s *.a *.o

