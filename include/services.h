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

/* Train Server */
#define TR_REG_NAME	"TR"
void trainserver();
void traincmdserver();
void trainsensorserver();
void trainclockserver();
