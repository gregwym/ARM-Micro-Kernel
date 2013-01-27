#ifndef __SYSCALL_H__
#define __SYSCALL_H__

int Create(int priority, void (*code) ());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
// int RegisterAs( char *name );
// int WhoIs( char *name );
int Send( int tid, char *msg, int msglen, char *reply, int replylen );
int Receive( int *tid, char *msg, int msglen );
int Reply( int tid, char *reply, int replylen );
#endif // __SYSCALL_H__
