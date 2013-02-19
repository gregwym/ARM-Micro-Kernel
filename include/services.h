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
int Puts(int channel, char *msg);

/* Train Server */
#define TR_REG_NAME	"TR"
void traincmdserver();
