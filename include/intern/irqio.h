#ifndef __IRQIO_H__
#define __IRQIO_H__

#define BUFFER_LEN 256

int sprintf( char *dst, char *format, ... );
void iprintf( int channel, unsigned int expect_len, char *format, ... );

#define uiprintf(com2_tid, line, column, fmt, args...) \
	iprintf(com2_tid, BUFFER_LEN, "\e[%d;%dH" fmt "\e[u", line, column, ##args)

#endif // __IRQIO_H__
