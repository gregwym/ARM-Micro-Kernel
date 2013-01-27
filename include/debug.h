#ifndef __DEBUG_H__
#define __DEBUG_H__

/*
 * Bit flags for DEBUG()
 */
#define DB_SYSCALL     0x001
#define DB_TASK        0x002
#define DB_MSG_PASSING 0x004
// #define DB_TRAIN_CTRL  0x008
// #define DB_THREADS     0x010
// #define DB_VM          0x020
// #define DB_EXEC        0x040
// #define DB_VFS         0x080
// #define DB_SFS         0x100
// #define DB_NET         0x200
// #define DB_NETFS       0x400
// #define DB_KMALLOC     0x800

#define dbflags 0 // DB_SYSCALL | DB_TASK

#define DEBUG(d, fmt, args...) (((dbflags) & (d)) ? bwprintf(COM2, fmt, ##args) : 0)

#endif // __DEBUG_H__
