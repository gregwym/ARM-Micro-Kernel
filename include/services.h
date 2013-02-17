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

void com1server();
void com2server();


/* Train Server */
#define Train_REG_NAME	"TR"
void SendToCOM1(int com1_server_tid, char c);
void SendToCOM2(int com2_server_tid, char c);
