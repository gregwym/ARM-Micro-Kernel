#ifndef __IRQIO_H__
#define __IRQIO_H__

int sprintf( char *dst, char *format, ... );
void iprintf( int channel, unsigned int expect_len, char *format, ... );

#endif // __IRQIO_H__
