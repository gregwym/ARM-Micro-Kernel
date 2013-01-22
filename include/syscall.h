#ifndef __SYSCALL_H__
#define __SYSCALL_H__

void Exit();
int Create(int priority, void (*code) ());

#endif // __SYSCALL_H__
