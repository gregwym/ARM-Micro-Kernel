#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <kern/unistd.h>

int Create(int priority, void (*code) ());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
int Send( int tid, char *msg, int msglen, char *reply, int replylen );
int Receive( int *tid, char *msg, int msglen );
int Reply( int tid, char *reply, int replylen );
int AwaitEvent( int eventid, char *event, int eventlen );
void Halt();

#endif // __UNISTD_H__
