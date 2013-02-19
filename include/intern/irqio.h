#ifndef __IRQIO_H__
#define __IRQIO_H__

#include <kern/ts7200.h>

#define ON	1
#define	OFF	0

int Getc( int channel );

int Putc( int channel, char ch);

void iprintf( int channel, char *format, ... );

#endif // __IRQIO_H__
