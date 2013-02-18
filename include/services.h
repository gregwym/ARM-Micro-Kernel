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
#define COM1_REG_NAME 	"C1"
#define COM2_REG_NAME 	"C2"

void comserver();
int Getc(int channel);
int Putc(int channel, char ch);


/* Train Server */
#define TR_REG_NAME	"TR"
void trainserver();
