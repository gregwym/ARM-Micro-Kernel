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
void com1server();
void com2server();
void SendToCOM1(int com1_server_tid, char c);
void SendToCOM2(int com2_server_tid, char c);


