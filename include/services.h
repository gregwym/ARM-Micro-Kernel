#ifndef __SERVICES_H__
#define __SERVICES_H__

/* Name Server */
void nameserver();
int RegisterAs( char *name );
int WhoIs( char *name );

/* Clock Server */
void clockserver();
int Delay( int ticks );
int DelayUntil( int ticks );
int Time( );

/* IO Server */
void comserver();
int Getc(int channel);
int Putc(int channel, char ch);
// set msglen to 0 to checkout string len by "strlen" automatically
int Puts(int channel, char *msg, int msglen);

#endif // __SERVICES_H__
