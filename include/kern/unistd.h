#ifndef _KERN_UNISTD_H_
#define _KERN_UNISTD_H_

/*
 * Constants for system calls
 */

#define STDIN			0	/* Standard input */
#define STDOUT			1	/* Standard output */
#define STDERR			2	/* Standard error */

/* Codes for reboot */
#define RB_REBOOT		0	/* Reboot system */
#define RB_HALT			1	/* Halt system and do not reboot */
#define RB_POWEROFF		2	/* Halt system and power off */

/* System-wide Event ID */
#define EVENT_TIME_ELAP	0	/* Time elapsed event */
#define EVENT_COM1_TX	1
#define EVENT_COM1_RX	2
#define EVENT_COM2_TX	3
#define EVENT_COM2_RX	4
#define EVENT_MAX		5

#endif // _KERN_UNISTD_H_
