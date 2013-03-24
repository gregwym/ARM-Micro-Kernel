#ifndef __DEBUG_H__
#define __DEBUG_H__

/*
 * Bit flags for DEBUG()
 */
#define DB_SYSCALL     0x001
#define DB_TASK        0x002
#define DB_CS          0x004
#define DB_NS          0x008
#define DB_RPS         0x010
#define DB_IRQ         0x020
#define DB_IO          0x040
#define DB_RESERVE     0x080
// #define DB_SFS         0x100
// #define DB_NET         0x200
// #define DB_NETFS       0x400
// #define DB_KMALLOC     0x800

#define dbflags 0 // DB_SYSCALL | DB_TASK | DB_CS | DB_NS | DB_RPS | DB_IRQ

#ifndef NDEBUG
	/* assert */
	#undef assert
	void _assert(char *, char *, char *);
	#define _STR(x) _VAL(x)
	#define _VAL(x) #x
	#define assert(test, msg)	((test) ? (void)0 \
		: _assert(__FILE__ ":" _STR(__LINE__), #test, msg))

	/* debug */
	#define DEBUG(d, fmt, args...) (((dbflags) & (d)) ? bwprintf(COM2, fmt, ##args) : 0)
	#define IDEBUG(d, com2_tid, line, column, fmt, args...) \
	(((dbflags) & (d)) ? iprintf(com2_tid, 256, "\e[s\e[%d;%dH" fmt "\e[u", line, column, ##args) : 0)
#else
	#define assert(test, msg)		((void)0)
	#define DEBUG(d, fmt, args...)	((void)0)
#endif // NDEBUG

#endif // __DEBUG_H__
